/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */

#include <assert.h>
#include <errno.h>
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <math.h>
#include <pthread.h>
#include <queue.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "fetch.h"
#include "grpc_server.h"
#include "logger.h"
#include "platform/platform.h"

static inline int
lock_ctx(struct ctx *ctx)
{
    int err = pthread_mutex_lock(&(ctx->mtx));
#if 0
    LOG_INFO("%p locked %p (%d)", __builtin_return_address(0), &(ctx->mtx),
              err);
#endif
    return err;
}

static inline void
unlock_ctx(struct ctx *ctx)
{
    (void)pthread_mutex_unlock(&(ctx->mtx));
#if 0
    LOG_INFO("%p unlocked %p", __builtin_return_address(0), &(ctx->mtx));
#endif
}

static inline int
wait_ctx(struct ctx *ctx)
{
    // to be called while &(ctx->mtx) was locked
    return pthread_cond_wait(&(ctx->cv), &(ctx->mtx));
}

static inline int
timedwait_ctx(struct ctx *ctx, lua_Number seconds)
{
    // to be called while &(ctx->mtx) was locked
    const clockid_t clock_id = CLOCK_MONOTONIC;
    struct timespec tp = {};
    if (clock_gettime(clock_id, &tp)) {
        return errno; // shall not happen
    }
    if (seconds > 0.0) {
        // ensure to return within 10[s] even if 'seconds' was too large
        const lua_Number limit_seconds = 10.0;
        if (limit_seconds > seconds) {
            // use as-is
        } else {
            // prevent sleeping too long
            seconds = limit_seconds;
        }
        const lua_Number base = floor(seconds);

        LOG_HIDDEN("pre  %ld.%09ld", (long)tp.tv_sec, tp.tv_nsec);

        tp.tv_sec += base;
        tp.tv_nsec += (seconds - base) * (lua_Number)1000000000.0;
        if (tp.tv_nsec > 1000L * 1000 * 1000) {
            tp.tv_sec += 1;
            tp.tv_nsec -= 1000L * 1000 * 1000;
        }
        LOG_HIDDEN("post %ld.%09ld", (long)tp.tv_sec, tp.tv_nsec);
    } else {
        // trigger re-lock if ctx->mtx
    }
    int ret = pthread_cond_timedwait(&(ctx->cv), &(ctx->mtx), &tp);
    if (ret && (ret != ETIMEDOUT)) {
        if (clock_gettime(clock_id, &tp) == 0) {
            LOG_WARN("%d ACT %ld.%09ld", ret, (long)tp.tv_sec, tp.tv_nsec);
        }
    }

    return ret;
}

static inline int
wakeup_ctx(struct ctx *ctx)
{
    // to be called while &(ctx->mtx) was locked
    LOG_HIDDEN("pthread_cond_broadcast %p", ctx);
    return pthread_cond_broadcast(&(ctx->cv));
}

// for LUA_REGISTRYINDEX
static char lua_registry_key[] = "dc_ctx";
static struct ctx *
get_ctx_from_lua(lua_State *L)
{
    struct ctx *ctx;
    const int top = lua_gettop(L);
    lua_pushlightuserdata(L, lua_registry_key);
    lua_gettable(L, LUA_REGISTRYINDEX);
    ctx = lua_touserdata(L, -1);
    lua_settop(L, top);
    return ctx;
}

// generate an entry in the DC-local async done queue
// to be called with a locked 'ctx'
static int
push_async_result(struct ctx *ctx, uint32_t async_tag, int code,
  const char *value, size_t result_size)
{
    int err = 0;
    struct async_op_state *ao = malloc(sizeof(*ao));
    if (ao) {
        memset(ao, 0, sizeof(*ao));
        ao->tag = async_tag;
        ao->result_code = (int)code;
        if (result_size > 0) {
            ao->result = strdup(value); // FIXME
            ao->size = result_size;
        }
        TAILQ_INSERT_TAIL(&(ctx->async.done_local), ao, next);
        wakeup_ctx(ctx);
        // 'ao' is owned by the queue
    } else {
        err = ENOMEM;
    }
    return err;
}

static int
init_pthread(struct ctx *ctx)
{
    int err;

    pthread_condattr_t cv_attr = {};
    err = pthread_condattr_init(&cv_attr);
    if (err) {
        return err;
    }
    err = pthread_condattr_setclock(&cv_attr, CLOCK_MONOTONIC);
    if (err) {
        (void)pthread_condattr_destroy(&cv_attr);
        return err;
    }

    err = pthread_mutex_init(&(ctx->mtx), NULL);
    if (err) {
        (void)pthread_condattr_destroy(&cv_attr);
        return err;
    }

    err = pthread_cond_init(&(ctx->cv), &cv_attr);
    (void)pthread_condattr_destroy(&cv_attr);
    if (err) {
        pthread_mutex_destroy(&(ctx->mtx));
        return err;
    }

    ctx->lock_ctx = lock_ctx;
    ctx->unlock_ctx = unlock_ctx;
    ctx->wait_ctx = wait_ctx;
    ctx->wakeup_ctx = wakeup_ctx;

    ctx->push_async_result = push_async_result;

    return (err);
}

static void
fini_pthread(struct ctx *ctx)
{
    pthread_cond_destroy(&(ctx->cv));
    pthread_mutex_destroy(&(ctx->mtx));
}

static void
dump_lua_error(lua_State *L, const char *errname)
{
    const char *errmsg = lua_tostring(L, -1);
    LOG_ERROR("Lua error[%s]:\n----\n"
              "%s"
              "\n----\n",
      errname, errmsg ? errmsg : "(?)");
}

static int
dostring(lua_State *L, const char *string)
{
    int err;
    const char *errname;

    int lerr = luaL_dostring(L, string);
    if (lerr == LUA_OK)
        return 0;
    switch (lerr) {
    case LUA_ERRRUN: // runtime error
        errname = "LUA_ERRRUN";
        err = EINVAL;
        break;
    case LUA_ERRMEM: // memory allocation error
        errname = "LUA_ERRMEM";
        err = ENOMEM;
        break;
    case LUA_ERRERR: // error in error handler
        errname = "LUA_ERRERR";
        err = EINVAL;
        break;
    case LUA_ERRSYNTAX:
        errname = "LUA_ERRSYNTAX";
        err = EINVAL;
        break;
    case LUA_YIELD: // coroutine yields
        errname = "LUA_YIELD";
        err = EINVAL;
        break;
    case LUA_ERRFILE: // file-related error
        errname = "LUA_ERRFILE";
        err = EIO;
        break;
    default:
        errname = "?";
        err = EINVAL;
        break;
    }
    dump_lua_error(L, errname);
    return err;
}

static int queue_update_event(struct ctx *ctx, const char *key,
  const char *value);

static int
setenv_from_lua(lua_State *L)
{
    int ret = 0;
    const int args = lua_gettop(L);
    const char *key = NULL;
    const char *value = NULL;
    struct ctx *ctx = get_ctx_from_lua(L);

    if (ctx && args == 2) {
        key = lua_tostring(L, 1);
        value = lua_tostring(L, 2);
        if (!key || !value) {
            LOG_ERROR("%p: trying to update %p by %p?", ctx, key, value);
            ret = -1;
        } else {
            ret = queue_update_event(ctx, key, value);
        }
    } else {
        LOG_ERROR("%p: args %d shall be 2", ctx, args);
        ret = -1; // FIXME
    }

    lua_pushinteger(L, ret); /* push result */
    return 1;                /* number of results */
}

static int
propagate_environment_from_lua(lua_State *L)
{
    int ret = 0;
    const int args = lua_gettop(L);
    const char *key = NULL;
    const char *value = NULL;
    struct ctx *ctx = get_ctx_from_lua(L);

    if (ctx && args == 2) {
        key = lua_tostring(L, 1);
        value = lua_tostring(L, 2);
        if (ctx->platform->propagate_environment) {
            // LOG_DEBUG(" propagate env update %s=%s", key, value);
            (ctx->platform->propagate_environment)(ctx->platform, key, value);
        } else {
            LOG_WARN(" no PAL func to handle env update %s=%s", key, value);
        }
    } else {
        LOG_ERROR("%p: args %d shall be 2", ctx, args);
        ret = -1; // FIXME
    }

    lua_pushinteger(L, ret); /* push result */
    return 1;                /* number of results */
}

#define FSM_ENV_LIMIT 64 // tentative
static int
gather_environment_from_lua(lua_State *L)
{
    int ret = 0;
    const int args = lua_gettop(L);
    const char *key = NULL;
    struct ctx *ctx = get_ctx_from_lua(L);
    char value[FSM_ENV_LIMIT] = {};
    size_t filled = 0;

    if (!ctx) {
        ret = EOPNOTSUPP;
    } else if (!(ctx->platform->gather_environment)) {
        ret = EOPNOTSUPP;
    } else if (args < 2) {
        LOG_ERROR("%p: args %d shall be >= 2", ctx, args);
        ret = EINVAL;
    } else {
        int isnum = 0;
        key = lua_tostring(L, 1);
        lua_Number previous = lua_tonumberx(L, 2, &isnum);
        if (isnum) {
            if (isnan(previous)) {
                LOG_DEBUG("last call for '%s'", key);
                ret = (ctx->platform->gather_environment)(ctx->platform, NULL,
                  key, NULL, 0, NULL);
            } else {
                lua_Number base = floor(previous);
                struct timespec tp = {
                    .tv_sec = (time_t)base,
                    .tv_nsec = (previous - base) * 1000000000.0,
                };
                LOG_DEBUG("call for '%s', previous=%f", key, previous);
                ret = (ctx->platform->gather_environment)(ctx->platform, &tp,
                  key, value, sizeof(value) - 1, &filled);
            }
        } else {
            LOG_DEBUG("first call for '%s'", key);
            ret = (ctx->platform->gather_environment)(ctx->platform, NULL, key,
              value, sizeof(value) - 1, &filled);
        }
    }
    if (ret == 0) {
        lua_pushlstring(L, value, filled);
        return 1;
    } else {
        lua_pushnil(L);
        lua_pushinteger(L, ret);
        return 2; /* nil, errobj */
    }
}

static int
generate_async_tag_from_lua(lua_State *L)
{
    uint32_t tag = TUS_DC_PAL_INVALID_ASYNC_TAG;

    struct ctx *ctx = get_ctx_from_lua(L);
    if (lock_ctx(ctx) == 0) {
        ctx->async.last_tag++;
        if (ctx->async.last_tag == TUS_DC_PAL_INVALID_ASYNC_TAG)
            ctx->async.last_tag++;
        tag = ctx->async.last_tag;
        unlock_ctx(ctx);
    }
    lua_pushinteger(L, tag); /* push result */
    return 1;                /* number of results */
}

static int
set_async_results_from_lua(lua_State *L)
{
    int err = 0;
    const int args = lua_gettop(L);
    uint32_t tag;
    lua_Integer code;
    size_t result_size = 0;
    const char *value = NULL;
    struct ctx *ctx = get_ctx_from_lua(L);

    if (!ctx || (args != 3 && args != 4)) {
        LOG_ERROR("%p: args %d", ctx, args);
        err = EINVAL;
    }

    // stack [1] is 'dc.async' object

    if (!err) {
        int isnum = 0;
        tag = lua_tointegerx(L, 2, &isnum);
        if (!isnum || (uint32_t)tag != tag) {
            LOG_ERROR("bad tag %ld", (long)tag);
            err = EINVAL;
        }
        LOG_DEBUG("got tag %u", (uint32_t)tag);
    }

    if (!err) {
        int isnum = 0;
        code = lua_tointegerx(L, 3, &isnum);
        if (!isnum || (int)code != code) {
            LOG_ERROR("bad code %ld (%d, %d)", (long)code, isnum, (int)code);
            err = EINVAL;
        }
    }

    if (!err && args == 4) {
        value = lua_tolstring(L, 4, &result_size);
        if (!value) {
            LOG_ERROR("failed to fetch result_text from args (%d)", args);
            err = EINVAL;
        }
    }

    if (!err) {
        LOG_DEBUG("tag=%u, code=%d", tag, (int)code);
        if (lock_ctx(ctx) == 0) {
            err = push_async_result(ctx, tag, code, value, result_size);
            unlock_ctx(ctx);
        } else {
            err = -1; //
        }
    }

    // ToDo; the caller may not able to habdle errors?
    LOG_DEBUG(" -> err=%d", err);
    if (err) {
        lua_pushinteger(L, err); /* push result */
        return 1;                /* number of results */
    }
    return 0; /* returns nil */
}

static void destroy_script_state_locked(struct ctx *ctx);

static int
get_version_from_lua(lua_State *L)
{
    int ltype;
    ltype = lua_type(L, 1);
    if (ltype != LUA_TSTRING) {
        LOG_ERROR("stack#1 != LUA_TSTRING (%d)", ltype);
        lua_pushnil(L);
        lua_pushinteger(L, EINVAL);
        return 2;
    }

    size_t limit = (4 << 10); // ToDo: tentative
    ltype = lua_type(L, 2);
    if (ltype == LUA_TNUMBER) {
        LOG_DEBUG("use default buffer size (%zd)", limit);
    } else if (ltype != LUA_TNUMBER) {
        LOG_ERROR("stack#2 != LUA_TNUMBER (%d)", ltype);
        lua_pushnil(L);
        lua_pushinteger(L, EINVAL);
        return 2;
    } else {
        limit = (size_t)lua_tointeger(L, 2);
        LOG_DEBUG("requested size limit = %zd)", limit);
    }

    enum dc_version_variation variation = DC_VERSION_VARIATION_CURRENT;
    ltype = lua_type(L, 3);
    if (ltype == LUA_TNIL) {
        // use default
    } else if (ltype == LUA_TNUMBER) {
        lua_Integer val = lua_tointeger(L, 3);
        if (val < 0 || DC_VERSION_CONTEXT__LIMIT <= val) {
            LOG_ERROR("stack#4: bad variation code(%lld)", (long long)val);
            lua_pushnil(L);
            lua_pushinteger(L, EINVAL);
            return 2;
        }
        variation = val;
    } else {
        // ToDo: allow to convert from a string
        LOG_ERROR("stack#3 != LUA_NUMBER (%d)", ltype);
        lua_pushnil(L);
        lua_pushinteger(L, EINVAL);
        return 2;
    }
    enum dc_version_context context = DC_VERSION_CONTEXT_ANY;
    ltype = lua_type(L, 4);
    if (ltype == LUA_TNIL) {
        // use default
    } else if (ltype == LUA_TNUMBER) {
        lua_Integer val = lua_tointeger(L, 4);
        if (val < 0 || DC_VERSION_CONTEXT__LIMIT <= val) {
            LOG_ERROR("stack#4: bad context code(%lld)", (long long)val);
            lua_pushnil(L);
            lua_pushinteger(L, EINVAL);
            return 2;
        }
        context = val;
    } else {
        // ToDo: allow to convert from a string
        LOG_ERROR("stack#4 != LUA_NUMBER (%d)", ltype);
        lua_pushnil(L);
        lua_pushinteger(L, EINVAL);
        return 2;
    }

    // ToDo: limit max size of buffer?
    void *buffer = calloc(1, limit);
    if (!buffer) {
        LOG_ERROR("calloc(%zu)", limit);
        lua_pushnil(L);
        lua_pushinteger(L, EINVAL);
        return 2;
    }

    size_t len = 0;
    const char *key = lua_tolstring(L, 1, &len);
    struct ctx *ctx = get_ctx_from_lua(L);
    int err = lock_ctx(ctx);
    if (err == 0) {
        size_t filled = 0;
        if (ctx->platform->get_version) {
            LOG_DEBUG(" get_version(key='%s', limit=%zd)", key, limit);
            err = (ctx->platform->get_version)(ctx->platform, variation,
              context, key, buffer, limit, &filled);
            if (!err && (filled > limit)) {
                LOG_ERROR("returned invalid size %zd", filled);
                err = EINVAL;
            }
        } else {
            LOG_WARN(" no PAL func to get version of '%s'", key);
            err = EOPNOTSUPP;
        }
        if (err) {
            free(buffer);
            lua_pushnil(L);
            lua_pushinteger(L, err);
            unlock_ctx(ctx);
            return 2;
        } else {
            lua_pushlstring(L, buffer, filled);
            free(buffer);
            unlock_ctx(ctx);
            return 1;
        }
    } else {
        // shall not happen actually
        free(buffer);
        LOG_ERROR("failed to lock? %d", err);
        return 0;
    }
}

static int
update_version_from_lua(lua_State *L)
{
    int ltype;
    ltype = lua_type(L, 1);
    if (ltype != LUA_TSTRING) {
        LOG_ERROR("stack#1 != LUA_TSTRING (%d)", ltype);
        lua_pushnil(L);
        lua_pushinteger(L, EINVAL);
        return 2;
    }
    ltype = lua_type(L, 2);
    if (ltype != LUA_TSTRING) {
        LOG_ERROR("stack#2 != LUA_TSTRING (%d)", ltype);
        lua_pushnil(L);
        lua_pushinteger(L, EINVAL);
        return 2;
    }
    enum dc_version_variation variation = DC_VERSION_VARIATION_CURRENT;
    ltype = lua_type(L, 3);
    if (ltype == LUA_TNIL) {
        // use default
    } else if (ltype == LUA_TNUMBER) {
        lua_Integer val = lua_tointeger(L, 3);
        if (val < 0 || DC_VERSION_CONTEXT__LIMIT <= val) {
            LOG_ERROR("stack#4: bad variation code(%lld)", (long long)val);
            lua_pushnil(L);
            lua_pushinteger(L, EINVAL);
            return 2;
        }
        variation = val;
    } else {
        // ToDo: allow to convert from a string
        LOG_ERROR("stack#3 != LUA_NUMBER (%d)", ltype);
        lua_pushnil(L);
        lua_pushinteger(L, EINVAL);
        return 2;
    }
    enum dc_version_context context = DC_VERSION_CONTEXT_ANY;
    ltype = lua_type(L, 4);
    if (ltype == LUA_TNIL) {
        // use default
    } else if (ltype == LUA_TNUMBER) {
        lua_Integer val = lua_tointeger(L, 4);
        if (val < 0 || DC_VERSION_CONTEXT__LIMIT <= val) {
            LOG_ERROR("stack#4: bad context code(%lld)", (long long)val);
            lua_pushnil(L);
            lua_pushinteger(L, EINVAL);
            return 2;
        }
        context = val;
    } else {
        // ToDo: allow to convert from a string
        LOG_ERROR("stack#4 != LUA_NUMBER (%d)", ltype);
        lua_pushnil(L);
        lua_pushinteger(L, EINVAL);
        return 2;
    }

    size_t size = 0;
    const char *key = lua_tostring(L, 1);
    const char *value = lua_tolstring(L, 2, &size);
    struct ctx *ctx = get_ctx_from_lua(L);
    int err = lock_ctx(ctx);
    if (err == 0) {
        if (ctx->platform->update_version) {
            LOG_DEBUG(" update_version(%s, %.16s...)", key, value);
            err = (ctx->platform->update_version)(ctx->platform, variation,
              context, key, value, size);
        } else {
            LOG_WARN(" no PAL func to set version of '%s'", key);
            err = EOPNOTSUPP;
        }
        if (err) {
            lua_pushinteger(L, err);
            unlock_ctx(ctx);
            return 1;
        } else {
            unlock_ctx(ctx);
            return 0;
        }
    } else {
        // shall not happen actually
        LOG_ERROR("failed to lock? %d", err);
        return 0;
    }
}

static int
call_updater_from_lua(lua_State *L)
{
    struct ctx *ctx = get_ctx_from_lua(L);

    int err = lock_ctx(ctx);
    if (err == 0) {
        // call_updater_from_lua(updater._native, key, ...)
        struct tus_dc_updater *u = lua_touserdata(L,
          1); // ToDo: switch to checkuserdata()?
        size_t len = 0;
        const char *name = luaL_tolstring(L, 2, &len);
        lua_pop(L, 1);
        const int num_args = lua_gettop(L) - 2;
        LOG_INFO("updater '%p', func '%s', num_args=%d", u, name, num_args);
        int ret = EINVAL;
        int pushed = 0;
        if (u) {
            for (size_t i = 0; i < u->num_functions; i++) {
                const struct tus_dc_updater_function *f = u->functions + i;
                LOG_INFO("- %zd: '%s' %p", i, f->name, f->func);
                if (strncmp(name, f->name, len + 1) == 0) {
                    struct tus_dc_updater_call_context call_ctx = {
                        .L = L,
                    };
                    ret = (f->func)(ctx->platform->util, &call_ctx);
                    pushed = call_ctx.returns;
                    LOG_INFO("- called with L=%p, ret=%d (pushed %d for Lua)",
                      L, ret, pushed);
                }
            }
        }
        if (ret) {
            lua_settop(L, num_args + 2);
            lua_pushnil(L);
            lua_pushinteger(L, ret);
            unlock_ctx(ctx);
            return 2;
        } else {
            unlock_ctx(ctx);
            return pushed;
        }
    } else {
        return 0; // cannot push safely
    }
}

static int
free_updater_from_lua(lua_State *L)
{
    struct ctx *ctx = get_ctx_from_lua(L);
    // free_updater_from_lua(updater._native)
    struct tus_dc_updater *u = lua_touserdata(L, 1);
    if (u) {
        // userdata is garbage-collected on Lua, omit lock since nobody can see
        // it
        LOG_INFO("drop custom updater %p", u);
        if (ctx->platform->free_updater) {
            (ctx->platform->free_updater)(ctx->platform, u);
        }
    } else {
        LOG_ERROR("cannot drop %p", u);
    }
    return 0;
}

static int
make_updater_from_lua(lua_State *L)
{
    struct ctx *ctx = get_ctx_from_lua(L);

    int err = lock_ctx(ctx);
    if (err == 0) {
        // -3 shal be a table to set '_methods'
        const char *updater_type = lua_tostring(L, -2);
        const char *target_id = lua_tostring(L, -1);
        LOG_INFO("updater_type '%s', target_id '%s' (%p)", updater_type,
          target_id, ctx->platform->get_updater);
        struct tus_dc_updater_type_id ut = {};
        strncpy(ut.id, updater_type, sizeof(ut.id) - 1); // fixme: tentative
        struct tus_dc_update_target_id tid = {};
        if (target_id) {
            strncpy(tid.id, target_id, sizeof(tid.id) - 1); // fixme: tentative
        }
        struct tus_dc_updater *u = lua_newuserdata(L, sizeof(*u));
        LOG_INFO("made a Lua userdata %p", u);
        if (ctx->platform->get_updater) {
            err = (ctx->platform->get_updater)(ctx->platform, &ut,
              (target_id ? &tid : NULL), u);
        } else {
            // no custom updater might be provided by PAL
            err = EOPNOTSUPP;
        }
        if (err) {
            lua_pushnil(L);
            lua_pushinteger(L, ENOENT);
            unlock_ctx(ctx);
            return 2;
        } else {
            // return pushed userdata
            unlock_ctx(ctx);
            return 1;
        }
    } else {
        lua_pushnil(L);
        lua_pushinteger(L, -1);
        return 2;
    }
}

static int
terminate_session_from_lua(lua_State *L)
{
    struct ctx *ctx = get_ctx_from_lua(L);

    int err = lock_ctx(ctx);
    if (err == 0) {
        const char *arg = lua_tostring(L, -1);
        if (strncmp(ctx->session_id, arg, sizeof(ctx->session_id)) == 0) {
            LOG_DEBUG("L %p(refs=%d) -> ctx %p, arg=%s", L, ctx->script.refs,
              ctx, arg);
            if (ctx->script.refs > 0) {
                ctx->script.pending_release = 1;
                wakeup_ctx(ctx); // unblock pop_update_event()
            } else {
                destroy_script_state_locked(ctx);
            }
        } else {
            LOG_DEBUG("L %p -> ctx %p, arg=%s != %s?", L, ctx, arg,
              ctx->session_id);
        }
        unlock_ctx(ctx);
    }
    return 0;
}

static int
dc_config_reload(struct ctx *ctx, lua_State *L)
{
    // load 'dc.config', 'dc shall be loaded at the top of stack
    const int dc_index = lua_gettop(L);

    int type_id = lua_getfield(L, -1, "config");
    if (type_id != LUA_TTABLE) {
        LOG_ERROR("missing 'config' in 'dc'? type id was %d, to be %d", type_id,
          LUA_TTABLE);
        lua_settop(L, dc_index);
        return EINVAL;
    }

    // call 'dc.config.reload(ctx->config_base_dir)'
    type_id = lua_getfield(L, -1, "_reload");
    if (type_id != LUA_TFUNCTION) {
        LOG_ERROR("missing 'reload' in 'config'? type id was %d, to be %d",
          type_id, LUA_TFUNCTION);
        lua_settop(L, dc_index);
        return EINVAL;
    }
    LOG_DEBUG("ctx->config_base_dir: %s", ctx->config_base_dir);
    lua_pushstring(L, ctx->config_base_dir);
    lua_call(L, 1, 0);

    lua_settop(L, dc_index);
    return 0;
}

// Get a ephemeral string from Lua and copy it to 'buf' if it was large enough.
// 'buf' shall be able to hold 'limit' bytes (including a terminator).
static int
copy_lua_string(lua_State *L, const char *key, char *buf, size_t limit,
  const char *fallback)
{
    int err = 0;
    size_t len = 0;
    switch (lua_getfield(L, -1, key)) {
    case LUA_TSTRING: {
        const char *value = luaL_checklstring(L, -1, &len);
        if (limit <= len) {
            LOG_ERROR("%s: %zu >= %zu", key, len, limit);
            return ENAMETOOLONG;
        }
        // note: '+1" is to copy '\0' at tail
        memcpy(buf, value, len + 1);
        LOG_DEBUG("%s: set as '%s'", key, buf);
        break;
    }
    case LUA_TNIL:
        if (fallback) {
            memset(buf, 0, limit);
            strncpy(buf, fallback, limit - 1);
            LOG_DEBUG("%s: fallback '%s'", key, buf);
        } else {
            err = ENOENT;
            LOG_ERROR("%s is mandated", key);
        }
        break;

    default:
        memset(buf, 0, limit);
        err = EINVAL;
        LOG_ERROR("%s shall be a string", key);
        break;
    }
    lua_remove(L, -1);

    return err;
}

static int
populate_lua_state(struct ctx *ctx, lua_State *L)
{
    int err;

    luaL_openlibs(L); // enable libraries. fixme: apply some restrictions?

    /* registry[dc_ctx] = ctx */
    lua_pushlightuserdata(L, lua_registry_key);
    lua_pushlightuserdata(L, ctx);
    lua_settable(L, LUA_REGISTRYINDEX);

    // LOG_DEBUG(" (stack top is %d)", lua_gettop(L));
    err = dostring(L, "dc = require('dc')");
    if (err)
        return err;

    // update the loaded moudle 'dc'
    // LOG_DEBUG(" (stack top is %d)", lua_gettop(L));
    int type_id = lua_getglobal(L, "dc");
    if (type_id != LUA_TTABLE) {
        LOG_ERROR("failed to load moudle 'dc'? type id was %d, to be %d",
          type_id, LUA_TTABLE);
        return EINVAL;
    }
    const int dc_index = lua_gettop(L);

    LOG_DEBUG("loaded moudle 'dc' at stack[%d]", dc_index);

    err = dc_config_reload(ctx, L);
    if (err) {
        return err;
    } else {
        // update 'ctx' based on loaded 'dc.config'
        type_id = lua_getfield(L, -1, "config");

        if (ctx->grpc.address[0]) {
            LOG_INFO("override grpc_server by '%s'", ctx->grpc.address);
        } else {
            err = copy_lua_string(L, "grpc_listen", ctx->grpc.address,
              sizeof(ctx->grpc.address), "0.0.0.0:50051");
            if (err) {
                return err;
            }
        }

        err = copy_lua_string(L, "id", ctx->domain_id, sizeof(ctx->domain_id),
          "");
        if (err) {
            return err;
        }

        LOG_INFO(" loaded config for domain_id: %s", ctx->domain_id);
    }

    if (ctx->domain_id) {
        // lua_pushstring() shall return an internal copy of the string (which
        // may be NULL?)
        lua_pushstring(L, ctx->domain_id);
        lua_setfield(L, dc_index, "id");
        lua_settop(L, dc_index);
        if (ctx->debug > 0) {
            dostring(L, "dc.log('new dc.id = ', dc.id)");
        }
    }

    LOG_DEBUG(" (stack top is %d)", lua_gettop(L));

    // set dc.*_from_lua
    lua_pushcfunction(L, get_version_from_lua);
    lua_setfield(L, dc_index, "get_version_from_lua");
    lua_pushcfunction(L, update_version_from_lua);
    lua_setfield(L, dc_index, "update_version_from_lua");

    lua_pushcfunction(L, make_updater_from_lua);
    lua_setfield(L, dc_index, "make_updater_from_lua");
    lua_pushcfunction(L, call_updater_from_lua);
    lua_setfield(L, dc_index, "call_updater_from_lua");
    lua_pushcfunction(L, free_updater_from_lua);
    lua_setfield(L, dc_index, "free_updater_from_lua");

    lua_pushcfunction(L, terminate_session_from_lua);
    lua_setfield(L, dc_index, "terminate_session_from_lua");

    if (http_push_fecher_to_lua(L) == 0) {
        // cFunction has been pushed to the stack
        lua_setfield(L, dc_index, "fetch_from_lua");
        if (ctx->debug > 0) {
            dostring(L,
              "dc.log('new dc.fetch_from_lua = ', dc.fetch_from_lua)");
        }
    }
    lua_settop(L, dc_index);
    return 0;
}

static int
wait_from_lua(lua_State *L)
{
    int ret = 0;
    struct ctx *ctx = get_ctx_from_lua(L);
    //
    const int idx_arg = 1;
    int num_tags = luaL_len(L, idx_arg);
    LOG_DEBUG("(num_tags=%d)", num_tags);

    lua_newtable(L);
    const int idx_ret = lua_gettop(L);
    int nonblock = 0;
    if (idx_ret > 2) {
        int ltype = lua_type(L, 2);
        if (ltype != LUA_TNIL) {
            // got 2nd arg 'timeout'
            nonblock = 1; // return immediately if no 'done' was queued
            LOG_HIDDEN("enable nonblock (%d)", ltype);
        } else {
            // called with timeout == nil
            LOG_HIDDEN("disble nonblock (%d)", ltype);
        }
    } else {
        // timeout has been omitted
        LOG_HIDDEN("blocking wait (%d)", idx_ret);
    }

    int found = 0;
    if (num_tags > 0) {
        lock_ctx(ctx);
        while (found == 0) {
            if (TAILQ_EMPTY(&(ctx->async.done_local))) {
                if (nonblock) {
                    // no 'done' in queue, force to return an empty table
                    break;
                }
                do {
                    wait_ctx(ctx);
                } while (TAILQ_EMPTY(&(ctx->async.done_local)));
            } else {
                // some 'done' events has been queued.
                // shall return a tag if matching one could be found,
                // but inihibit looping.
                found = 1;
            }
            for (int ti = 1; ti <= num_tags; ti++) {
                lua_rawgeti(L, idx_arg, ti);

                int isnum = 0;
                lua_Integer atag = lua_tointegerx(L, -1, &isnum);
                if (!isnum || atag < 0 || UINT32_MAX < atag) {
                    LOG_ERROR("got non-tag as arg[%d]?", ti);
                    continue;
                }
                lua_pop(L, 1);
                LOG_DEBUG("- arg[%d] = %u", ti, (uint32_t)atag);

                {
                    struct async_op_state *p = NULL;
                    TAILQ_FOREACH (p, &(ctx->async.done_local), next) {
                        // LOG_DEBUG("try: %d", p->tag);
                        if (p->tag == (uint32_t)atag) {
                            LOG_DEBUG("matchedf %u", (uint32_t)atag);
                            lua_newtable(L);

                            lua_pushinteger(L, p->result_code);
                            lua_setfield(L, -2, "code");

                            if (p->size > 0) {
                                lua_pushlstring(L, p->result, p->size);
                                lua_setfield(L, -2, "result");
                            }
                            lua_seti(L, idx_ret, atag);

                            lua_settop(L, idx_ret);

                            found++;
                            break;
                        }
                    }
                }
            }
        }
        unlock_ctx(ctx);
    }

    ret = 0;
    if (!ret) {
        // stack top shall be a ('completed') table to return
        assert(idx_ret == lua_gettop(L));
        return 1; /* number of results */
    } else {
        /* push "nil, error_obj" */
        lua_pushnil(L);
        lua_pushinteger(L, ret);
        return 2; /* number of results */
    }
}

static int
drop_from_lua(lua_State *L)
{
    int ret = 0;
    struct ctx *ctx = get_ctx_from_lua(L);
    if (lock_ctx(ctx) == 0) {
        int isnum = 0;
        lua_Integer atag = lua_tointegerx(L, 1, &isnum);
        if (!isnum || atag < 0 || UINT32_MAX < atag) {
            LOG_ERROR("got non-tag %ld?", (long)atag);
            ret = EINVAL;
        }
        if (!ret) {
            lua_pop(L, 1);
            LOG_DEBUG("- arg = %u", (uint32_t)atag);

            struct async_op_state *p = NULL;
            TAILQ_FOREACH (p, &(ctx->async.done_local), next) {
                if (p->tag == atag)
                    break;
            }
            if (p) {
                LOG_DEBUG("- %p", p);
                TAILQ_REMOVE(&(ctx->async.done_local), p, next);
                wakeup_ctx(ctx);
            } else {
                LOG_DEBUG("- tag %u is not known", (uint32_t)atag);
                ret = ENOENT;
            }
        }
        unlock_ctx(ctx);
    }
    if (!ret) {
        return 0; /* number of results */
    } else {
        /* push "error_obj" */
        lua_pushinteger(L, ret);
        return 1; /* number of results */
    }
}

static int
clock_gettime__common(lua_State *L, clockid_t clock_id)
{
    int ret;
    struct timespec tp = {};
    ret = clock_gettime(clock_id, &tp);
    assert(ret == 0); // ToDo: require alternate implementation?

    // lua_Number is expected to be 'double' or 'float',
    // either shall have enough precision for normal use-cases
    lua_Number n = ((lua_Number)tp.tv_sec +
      tp.tv_nsec / (lua_Number)1000000000.0);
    LOG_HIDDEN("[%d] %ld.%09ld -> %.9f", (int)clock_id, (long)tp.tv_sec,
      tp.tv_nsec, (double)n);
    lua_pushnumber(L, n);
    return 1;
}

static int
clock_gettime_monotonic(lua_State *L)
{
    return clock_gettime__common(L, CLOCK_MONOTONIC);
}

// wall-clock time
static int
clock_gettime_realtime(lua_State *L)
{
    return clock_gettime__common(L, CLOCK_REALTIME);
}

static int
set_next_wakeup_from_lua(lua_State *L)
{
    int ret = 0;
    struct ctx *ctx = get_ctx_from_lua(L);

    if (ctx && (ctx->script.L == L)) {
        lua_Number next_wakeup;
        switch (lua_type(L, -1)) {
        case LUA_TNIL:
            next_wakeup = nan("");
            break;
        case LUA_TNUMBER:
            // number of seconds to sleep
            next_wakeup = lua_tonumber(L, -1);
            break;
        default:
            ret = EINVAL;
        }
        if (!ret) {
            if (isnan(next_wakeup)) {
                LOG_DEBUG("keep next_wakeup = %f", ctx->script.next_wakeup);
            } else {
                if (next_wakeup > ctx->script.next_wakeup) {
                    LOG_DEBUG("keep next_wakeup = %f (ignore %f)",
                      ctx->script.next_wakeup, next_wakeup);
                } else {
                    LOG_DEBUG("update next_wakeup = %f (was %f)", next_wakeup,
                      ctx->script.next_wakeup);
                    lock_ctx(ctx);
                    ctx->script.next_wakeup = next_wakeup;
                    wakeup_ctx(ctx); // unblock loop() to trigger next try
                    unlock_ctx(ctx);
                }
            }
        } else {
            LOG_ERROR("%d", ret);
        }
    }
    if (ret) {
        lua_pushinteger(L, ret); /* push as an error object */
        return 1;                /* number of results */
    } else {
        return 0;
    }
}

// additinal components tunder 'dc' for TUP script runtime
static int
populate_lua_state_for_runtime(struct ctx *ctx, lua_State *L)
{
    int err;
    // load 'tup_parser' as a global
    err = dostring(L, "tup_parser = require(\"tup_parser\")");
    if (err) {
        return err;
    }

    // update the loaded moudle 'dc'
    err = dostring(L, "dc.async = require('async');dc.async.parent=dc;");
    if (err)
        return err;

    // LOG_DEBUG(" (stack top is %d)", lua_gettop(L));
    int type_id = lua_getglobal(L, "dc");
    if (type_id != LUA_TTABLE) {
        LOG_ERROR("failed to load moudle 'dc'? type id was %d, to be %d",
          type_id, LUA_TTABLE);
        return EINVAL;
    }
    const int dc_index = lua_gettop(L);

    // export dc.*
    // ToDo: better to add as top-level functions ?
    {
        lua_pushcfunction(L, clock_gettime_monotonic);
        lua_setfield(L, dc_index, "clock_gettime_monotonic");
        lua_pushcfunction(L, clock_gettime_realtime);
        lua_setfield(L, dc_index, "clock_gettime_realtime");
        lua_pushcfunction(L, set_next_wakeup_from_lua);
        lua_setfield(L, dc_index, "set_next_wakeup_from_lua");

        // dostring(L, "print('dc.clock_gettime_monotonic()',
        // dc.clock_gettime_monotonic());"); dostring(L,
        // "print('clock_gettime_realtime()', dc.clock_gettime_realtime());");
        lua_settop(L, dc_index);
    }

    // allow delta-update module to request periodic calls of
    // dc.delta_update.resume_coroutines()
    err = dostring(L,
      "dc.delta_update = require('delta_update');"
      "dc.delta_update.init(dc.set_next_wakeup_from_lua, dc.log);"
      "print('====================================', dc.delta_update, dc.delta_update.request_resume)");
    if (err) {
        LOG_ERROR(
          "failed to init dc.delta_update (%d), missing Lua extentions?", err);
        return err;
    }

    type_id = lua_getfield(L, dc_index, "fsm");
    if (type_id != LUA_TTABLE) {
        LOG_ERROR("missing 'dc.fsm'? type id was %d, to be %d", type_id,
          LUA_TTABLE);
        return EINVAL;
    }

    const int fsm_index = lua_gettop(L);
    type_id = lua_getfield(L, fsm_index, "dbg");
    if (type_id != LUA_TSTRING) {
        LOG_ERROR("missing 'dc.fsm.dbg'? type id was %d, to be %d", type_id,
          LUA_TSTRING);
        return EINVAL;
    }
    const char *dbg = lua_tostring(L, -1);
    fprintf(stderr, "(!! fsm.dbg = '%s')", dbg);

    {
        // inject DC FMS-runtime's custom features
        // ToDo: better to register under a dedicated table, like
        // DC._native.xxx()?
        lua_pushcfunction(L, setenv_from_lua);
        lua_setfield(L, fsm_index, "set_environment_native");
        lua_pushcfunction(L, propagate_environment_from_lua);
        lua_setfield(L, fsm_index, "propagate_environment_native");
        lua_pushcfunction(L, gather_environment_from_lua);
        lua_setfield(L, fsm_index, "gather_environment_native");
        lua_settop(L, fsm_index);
    }

    if (1) {
        lua_getglobal(L, "dc");
        lua_getfield(L, -1, "async");
        lua_pushcfunction(L, wait_from_lua);
        lua_setfield(L, -2, "wait_from_lua");
        lua_pushcfunction(L, drop_from_lua);
        lua_setfield(L, -2, "drop_from_lua");
        lua_pushcfunction(L, set_async_results_from_lua);
        lua_setfield(L, -2, "set_async_results_from_lua");
        lua_pushcfunction(L, generate_async_tag_from_lua);
        lua_setfield(L, -2, "generate_async_tag_from_lua");
    }
    lua_settop(L, 1);

    return 0;
}

static int
init_lua_dcmain(struct ctx *ctx)
{
    int err;
    if (ctx->L) {
        LOG_ERROR("ctx->L is already set %p", ctx->L);
        return EINVAL;
    }
    // ToDo: prefer lua_newstate() to use custom allocators?
    ctx->L = luaL_newstate();
    if (!(ctx->L)) {
        LOG_ERROR("failed to create new Lua state (%p)", ctx->L);
        return EINVAL;
    }
    err = populate_lua_state(ctx, ctx->L);
    if (err) {
        lua_close(ctx->L);
        ctx->L = NULL;
    }
    return err;
}

static lua_State *
get_script_state_locked(struct ctx *ctx, const char *session_id)
{
    lua_State *L = NULL;

    if (ctx) {
        // lifetime of ctx->script.L is bounded by its initator (an UO script)
        if (!ctx->script.pending_release)
            L = ctx->script.L;
        if (L) {
            if (strncmp(ctx->session_id, session_id, sizeof(ctx->session_id)) !=
              0) {
                LOG_ERROR("mismatched session id cur '%s' is not '%s'",
                  ctx->session_id, session_id);
                // FIXME: drop users (script.refs) and re-gen new state?
                L = NULL;
            } else {
                while (ctx->script.refs > 0) {
                    LOG_HIDDEN("grabbing DC script state %d", ctx->script.refs);
                    wait_ctx(ctx);
                }
                ctx->script.refs++;
                // note: ctx->script.refs is expected to be 1 fow now
                LOG_DEBUG("use script.L = %p (refs=%d) for session '%s'", L,
                  ctx->script.refs, session_id);
            }
        } else {
            L = luaL_newstate();
            if (L) {
                int err = populate_lua_state(ctx, L);
                if (!err) {
                    // enable dc.fsm etc.
                    err = populate_lua_state_for_runtime(ctx, L);
                }
                if (err) {
                    LOG_DEBUG("(failed to init Lua state %p)", L);
                    lua_close(L);
                    L = NULL;
                }
                if (L) {
                    strncpy(ctx->session_id, session_id,
                      sizeof(ctx->session_id) - 1);
                    ctx->script.L = L;
                    ctx->script.refs = 1;
                    ctx->script.next_wakeup = nan("");
                    wakeup_ctx(ctx);
                    LOG_DEBUG("(new Lua stete %p for '%s')", L,
                      ctx->session_id);

                    if (ctx->platform->notify) {
                        (ctx->platform->notify)(ctx->platform,
                          TUS_DC_PAL_NOTIFICATION_SCRIPT_STARTED, NULL, 0);
                    }
                } else {
                    LOG_DEBUG("(failed to init script Lua %p)", L);
                }
            }
        }
    }
    LOG_HIDDEN("return Lua state %p", L);
    return L;
}

lua_State *
get_script_state(struct ctx *ctx, const char *session_id)
{
    if (!session_id) {
        // (like GetVersion())

        LOG_DEBUG("use DC main context %p", ctx->L);
        return ctx->L; // lifetime of ctx->L is almost static
    }
    // LOG_DEBUG("locking DC ctx %p for session '%s'", ctx, session_id);
    lua_State *L = NULL;
    if (lock_ctx(ctx) == 0) {
        L = get_script_state_locked(ctx, session_id);
        unlock_ctx(ctx);
    }
    return L;
}

static void
destroy_script_state_locked(struct ctx *ctx)
{
    // to be called with ctx->mtx locked
    lua_State *L = ctx->script.L;
    if (L) {
        if (ctx->script.refs == 0) {
            LOG_DEBUG("(drop Ls %p '%s')\n", L, ctx->session_id);
            ctx->script.pending_release = 0;
            ctx->script.refs = 0;
            ctx->script.L = NULL;
            memset(ctx->session_id, 0, sizeof(ctx->session_id));
            lua_close(L);

            if (ctx->platform->notify) {
                // ToDo: pack error information if terminated abnormally?
                void *aux = NULL;
                (ctx->platform->notify)(ctx->platform,
                  TUS_DC_PAL_NOTIFICATION_SCRIPT_STOPPED, aux, 0);
            }
        }
        wakeup_ctx(ctx); // unblock pop_update_event()
    }
}

static void
release_script_state_locked(struct ctx *ctx, lua_State *L)
{
    if (ctx) {
        if (L && ctx->script.L == L) {
            ctx->script.refs--;
            LOG_DEBUG("released ctx->script.L, refs=%d '%s')", ctx->script.refs,
              ctx->session_id);
            if (ctx->script.refs <= 0) {
                // 'ready to release'
                if (ctx->script.pending_release) {
                    destroy_script_state_locked(ctx);
                }
            }
        } else {
            if (ctx->script.L) {
                LOG_DEBUG("released ctx->script.L %p != %p? %d", ctx->script.L,
                  L, ctx->script.refs);
            }
        }
        wakeup_ctx(ctx);
    }
}

void
release_script_state(lua_State *L)
{
    struct ctx *ctx = get_ctx_from_lua(L);
    if (lock_ctx(ctx) == 0) {
        release_script_state_locked(ctx, L);
        unlock_ctx(ctx);
    }
}

static void
fini_ctx(struct ctx *ctx)
{
    if (lock_ctx(ctx) == 0) {
        LOG_DEBUG("ctx->shutdown=%d", ctx->shutdown);
        ctx->shutdown = 1;
        wakeup_ctx(ctx);

        // allow worker threads to advance
        unlock_ctx(ctx);

        if (ctx->async.enabled) {
            (void)pthread_join(ctx->async.th, NULL);
        }

        lock_ctx(ctx);

        // no more worker which interacts witl ctx->L
        if (ctx->L) {
            lua_close(ctx->L);
        }
        unlock_ctx(ctx);
    }

    // cleanup PAL
    if (ctx->platform && ctx->platform->finalize) {
        (ctx->platform->finalize)(ctx->platform);
    }

    fini_pthread(ctx);
}

static int
init_grpc_server(struct ctx *ctx)
{
    LOG_DEBUG("stating gRPC server %d", ctx->grpc.running);
    pthread_attr_t *attr = NULL;
    lock_ctx(ctx);
    int err = pthread_create(&(ctx->grpc.th), attr, grpc_server_worker, ctx);
    if (err) {
        LOG_ERROR("failed to create gRPC server thead: %d", err);
        unlock_ctx(ctx);
        return err;
    }
    while (ctx->grpc.running == 0) {
        if (ctx->grpc.error) {
            LOG_ERROR("failed to start gRPC server: %d", ctx->grpc.error);
            unlock_ctx(ctx);
            return ctx->grpc.error;
        }
        LOG_DEBUG("waiting gRPC server %d", ctx->grpc.running);

        wait_ctx(ctx);
    }
    LOG_DEBUG("stated gRPC server %d", ctx->grpc.running);
    unlock_ctx(ctx);

    return 0;
}

static int init_async_executor(struct ctx *ctx);

// assuming only one PAL may exist for for each DC for now
static struct tus_dc_platform dc_platform;

static int
init_ctx(struct ctx *ctx)
{
    int err;

    LOG_DEBUG("initializing ctx(%p)", ctx);

    err = init_pthread(ctx);
    if (err) {
        LOG_DEBUG("failed to init pthread %d", err);
        abort(); // trigger core-dump
    } else {
        // initialize pthread
    }

    // init dc_platform.util for tus_dc_platform_initialize()
    err = prepare_dc_platform_utility(&dc_platform, ctx);
    if (err) {
        LOG_ERROR("prepare_dc_platform_utility() failed %d", err);
        return err;
    }

    err = tus_dc_platform_initialize(&dc_platform);
    if (err) {
        LOG_ERROR("tus_dc_platform_initialize() failed %d", err);
        return err;
    }

    ctx->platform = &dc_platform;

    SIMPLEQ_INIT(&(ctx->queued_updates));
    TAILQ_INIT(&(ctx->async.pending_exec));
    TAILQ_INIT(&(ctx->async.done_uo));
    TAILQ_INIT(&(ctx->async.done_local));

    // init cURL
    err = http_init();
    if (err) {
        LOG_ERROR("failed to init HTTP fetcher %d", err);
        return err;
    }
    err = init_async_executor(ctx);
    if (err)
        return err;

    err = init_lua_dcmain(ctx);
    if (err)
        return err;

    err = init_grpc_server(ctx);
    if (err)
        return err;

    LOG_DEBUG("initialized ctx %p", ctx);

    return (err);
}

int
queue_async_op(struct ctx *ctx, void *arg, const char *session_id,
  void (*release)(struct ctx *ctx, struct async_op_state *as),
  void (*run)(struct ctx *ctx, struct async_op_state *as), uint32_t tag)
{
    int err;
    if (!ctx || !arg || !tag)
        return (EINVAL);

    // ToDo: pre-allocate a pool?
    struct async_op_state *ao = malloc(sizeof(*ao));
    if (!ao)
        return ENOMEM;
    memset(ao, 0, sizeof(*ao));

    ao->tag = tag;
    ao->arg = arg;
    ao->release = release;
    ao->run = run;
    ao->from_uo = 1;
    strncpy(ao->session_id, session_id, sizeof(ao->session_id) - 1);

    err = lock_ctx(ctx);
    if (err == 0) {
        TAILQ_INSERT_TAIL(&(ctx->async.pending_exec), ao, next);
        LOG_DEBUG("queue %x", tag);
        wakeup_ctx(ctx);
        unlock_ctx(ctx);
    } else {
        free(ao);
    }
    return err;
}

// wait for UO notifications
int
wait_update_event(struct ctx *ctx, const char *session_id)
{
    int err = lock_ctx(ctx);
    if (!err) {
        while (SIMPLEQ_EMPTY(&(ctx->queued_updates)) &&
          TAILQ_EMPTY(&(ctx->async.done_uo))) {
            if (strncmp(ctx->session_id, session_id, sizeof(ctx->session_id)) !=
              0) {
                LOG_DEBUG("%s != %s, deblock", ctx->session_id, session_id);
                err = EINTR;
                break;
            }
            wait_ctx(ctx);
        }
        unlock_ctx(ctx);
    }
    return err;
}

struct update_event *
pop_update_event(struct ctx *ctx)
{
    struct update_event *ue = NULL;
    int err = lock_ctx(ctx);

    if (err == 0) {
        if (!SIMPLEQ_EMPTY(&(ctx->queued_updates))) {
            ue = SIMPLEQ_FIRST(&(ctx->queued_updates));
            SIMPLEQ_REMOVE_HEAD(&(ctx->queued_updates), next);
            LOG_DEBUG("%p: %s = %s", ue, ue->key, ue->value);
        }
        unlock_ctx(ctx);
    }
    return ue;
}

struct async_op_state *
pop_async_done_uo(struct ctx *ctx)
{
    struct async_op_state *ao = NULL;
    int err = lock_ctx(ctx);
    if (err == 0) {
        if (!TAILQ_EMPTY(&(ctx->async.done_uo))) {
            ao = TAILQ_FIRST(&(ctx->async.done_uo));
            TAILQ_REMOVE(&(ctx->async.done_uo), ao, next);
            LOG_DEBUG("%p: tag=%d, result_code=%d", ao, ao->tag,
              ao->result_code);
        }
        unlock_ctx(ctx);
    }
    return ao;
}

int
queue_update_event(struct ctx *ctx, const char *key, const char *value)
{
    int err;
    if (!ctx || !key || !value)
        return (EINVAL);

    // ToDo: pre-allocate a pool?
    struct update_event *ue = malloc(sizeof(*ue));
    if (!ue)
        return ENOMEM;
    memset(ue, 0, sizeof(*ue));

    err = lock_ctx(ctx);
    if (err == 0) {
        strncpy(ue->key, key, sizeof(ue->key) - 1);
        strncpy(ue->value, value, sizeof(ue->value) - 1);
        SIMPLEQ_INSERT_TAIL(&(ctx->queued_updates), ue, next);
        LOG_DEBUG("enQ '%s=%s'", ue->key, ue->value);
        wakeup_ctx(ctx);
        unlock_ctx(ctx);
    } else {
        free(ue);
    }
    return err;
}

// consume ops queued by queue_async_op() and
// update
static void *
async_executor(void *arg)
{
    struct ctx *ctx = arg;
    // LOG_DEBUG("init for (%p)", ctx);

    int err = lock_ctx(ctx);
    ctx->async.enabled = 1;
    LOG_DEBUG("stared for (%p)", ctx);
    if (err == 0) {
        while (!ctx->shutdown) {
            struct async_op_state *ao;
            ao = TAILQ_FIRST(&(ctx->async.pending_exec));
            if (!ao) {
                LOG_DEBUG("blocking (shutdown=%d)", ctx->shutdown);
                wait_ctx(ctx);
                continue;
            }
            LOG_DEBUG("process (%p)", ao);
            TAILQ_REMOVE(&(ctx->async.pending_exec), ao, next);
            if (ao->run) {
                unlock_ctx(ctx);
                (ao->run)(ctx, ao);
                lock_ctx(ctx);
            }
            if (ao->from_uo) {
                // move to 'done' queue which is monitored by gRPC
                TAILQ_INSERT_TAIL(&(ctx->async.done_uo), ao, next);
            } else {
                // queue which is copsumed by DC-local wait()
                TAILQ_INSERT_TAIL(&(ctx->async.done_local), ao, next);
            }
        }
        unlock_ctx(ctx);
    }
    LOG_DEBUG("exit from %s", __func__);
    return NULL;
}

static int
init_async_executor(struct ctx *ctx)
{
    pthread_attr_t *attr = NULL;
    int err = pthread_create(&(ctx->async.th), attr, async_executor, ctx);
    if (err)
        return err;

    return 0;
}

static int
loop(struct ctx *ctx)
{
    int err;

    err = init_ctx(ctx);
    if (err)
        return err;
    lock_ctx(ctx);
    // periodically update Lua state
    do {
        if (ctx->script.L) {
            LOG_DEBUG("ctx->script.next_wakeup is %f", ctx->script.next_wakeup);
            if (isnan(ctx->script.next_wakeup)) {
                // no need to wakeup
                wait_ctx(ctx);
            } else {
                timedwait_ctx(ctx, ctx->script.next_wakeup);
            }
            while (ctx->script.L) {
                double next_wakeup = nan("");
                lua_State *L = get_script_state_locked(ctx, ctx->session_id);
                LOG_HIDDEN("grabbed %p", L);
                unlock_ctx(ctx);
                int top = lua_gettop(L);
                int lerr = luaL_loadstring(L,
                  "return dc.fsm:sync_from_externals(dc.clock_gettime_monotonic())");
                if (lerr != LUA_OK) {
                    LOG_ERROR("sync_from_externals() -> %d", lerr);
                    break;
                }
                lerr = lua_pcall(L, 0, 3, 0);
                if (lerr == LUA_OK) {
                    err = 0;
                    const char *key = lua_tostring(L, -3);
                    const char *value = lua_tostring(L, -2);
                    if (value == NULL) {
                        LOG_HIDDEN("key '%s' is not yet ready", key);
                        // check 'wait how long'
                        switch (lua_type(L, -1)) {
                        case LUA_TNIL:
                            // no need to call again
                            LOG_HIDDEN("done(%d)", lua_type(L, -1));
                            break;
                        case LUA_TNUMBER:
                            // number of seconds to sleep
                            next_wakeup = lua_tonumber(L, -1);
                            LOG_HIDDEN("lua_tonumber(L, -1) = %f", next_wakeup);
                            break;
                        default:
                            // shall not happen
                            LOG_ERROR("bad type(%d) '%s'?", lua_type(L, -1),
                              lua_tostring(L, -1));
                            break;
                        }
                    } else {
                        LOG_INFO("got '%s = %s'", key, value);
                        lua_getglobal(L, "dc");
                        lua_getfield(L, -1, "fsm");
                        lua_getfield(L, -1, "set_environment");
                        lua_getfield(L, -3, "fsm");

                        lua_pushstring(L, key);
                        lua_pushstring(L, value);
                        if (lua_pcall(L, 3, 1, 0) != LUA_OK) {
                            LOG_ERROR("set_environment(%s, %s) failed?", key,
                              value);
                        }
                        next_wakeup = 0.0; // should try another key
                    }
                } else {
                    // shall map lerr to a TUS error code
                    const char *errmsg = lua_tostring(L, -1);
                    LOG_ERROR("(lerr=%d)\n----\n"
                              "%s"
                              "\n----\n",
                      lerr, errmsg ? errmsg : "(?)");
                }
                lua_settop(L, top);

                {
                    int lerr = luaL_loadstring(L,
                      "return dc.delta_update.resume_coroutines(dc.clock_gettime_monotonic())");
                    if (lerr != LUA_OK) {
                        LOG_ERROR("tried to resume_coroutines() -> %d", lerr);
                    } else {
                        lerr = lua_pcall(L, 0, 1, 0);
                        if (lerr == LUA_OK) {
                            err = 0;
                            int call_again = lua_toboolean(L, -1);
                            if (call_again) {
                                LOG_DEBUG(
                                  "call_again=%d, force next_wakeup(%f) be 0.0",
                                  call_again, next_wakeup);
                                next_wakeup = 0.0;
                            } else {
                                LOG_DEBUG("call_again=%d, keep next_wakeup(%f)",
                                  call_again, next_wakeup);
                                // no need to override
                            }
                        } else {
                            dump_lua_error(L, "resume_coroutines()");
                        }
                    }
                }

                lock_ctx(ctx);
                ctx->script.next_wakeup = next_wakeup;
                release_script_state_locked(ctx, L);
                if (isnan(next_wakeup) || next_wakeup > 0.0) {
                    LOG_HIDDEN("trigger sleep %f", next_wakeup);
                    break;
                }
            }
        } else {
            LOG_DEBUG("no DC script is runnig %p %f", ctx->script.L,
              ctx->script.next_wakeup);
            wait_ctx(ctx);
        }
    } while (!ctx->shutdown); // fixme: nobody will set ctx->shutdown (yet)
    unlock_ctx(ctx);

    // ToDo: call server->Shutdown() when appropreate
    (void)pthread_join(ctx->grpc.th, NULL);

    return err;
}

static void
usage(const char *prog)
{
    fprintf(stderr,
      "Usage: %s [-c CONFIG_BASE_DIR] [-l GRPC_TARGET]\n"
      "\n"
      "OPTIONS\n"
      " -c CONFIG_BASE_DIR: a path of a directory to read 'config'"
      "  (default = the executable's dir)\n"
      " -l GRPC_TARGET: where gRPC server shall listen on"
      "  (default = '0.0.0.0:50051')\n",
      prog);
}

static int
parse_args(struct ctx *ctx, int argc, char **argv)
{
    int opt;
    char *prog = (argc > 0 && argv[0] && *argv[0]) ? argv[0] : "";
    const char *p = strrchr(prog, '/');
    if (p) {
        char base_dir[PATH_MAX] = {};
        if (p - prog > (ssize_t)sizeof(base_dir)) {
            return ENAMETOOLONG;
        }
        memcpy(base_dir, prog, p - prog);
        if (chdir(base_dir)) {
            int err = errno;
            perror(base_dir);
            return err;
        }
        LOG_DEBUG("chdir('%s')", base_dir);
    }
    while ((opt = getopt(argc, argv, "hc:l:")) != -1) {
        switch (opt) {
        case 'c':
            memset(ctx->config_base_dir, 0, sizeof(ctx->config_base_dir));
            strncpy(ctx->config_base_dir, optarg,
              sizeof(ctx->config_base_dir) - 1);
            LOG_DEBUG("config_base_dir : '%s'", ctx->config_base_dir);
            break;
        case 'l': // gRPC server listen
            memset(ctx->grpc.address, 0, sizeof(ctx->grpc.address));
            strncpy(ctx->grpc.address, optarg, sizeof(ctx->grpc.address) - 1);
            LOG_DEBUG("grpc_server : '%s'", ctx->grpc.address);
            break;
        case 'h':
            usage(prog);
            exit(EXIT_SUCCESS); // usage() only
        default:
            usage(prog);
            return -1;
        }
    }

    return 0;
}

int
main(int argc, char **argv)
{
    struct ctx ctx = {};

    int err = parse_args(&ctx, argc, argv);
    if (err) {
        LOG_ERROR("parse_args failed(%d)", err);
        exit(EXIT_FAILURE);
    }

    ctx.debug = 1;

    if (!(ctx.config_base_dir[0])) {
        memset(ctx.config_base_dir, 0, sizeof(ctx.config_base_dir));
        ctx.config_base_dir[0] = '.';
        LOG_DEBUG("- use default config_base_dir : '%s'", ctx.config_base_dir);
    }

    err = loop(&ctx);

    fini_ctx(&ctx);

    if (err)
        exit(EXIT_FAILURE);

    return 0;
}
