/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */

#include <delta_update.h>
#include <errno.h>
#include <lauxlib.h>
#include <logger.h>
#include <lua.h>
#include <lualib.h>
#include <stdio.h>
#include <string.h>

_Static_assert(TUS_DELTA_UPDATE_PATCH_OK == 0, "broken API");

static const char CONTEXT[] = "delta_update_native";
static int verbosity = 0;

static void __attribute__((used))
dump_lua_stack(lua_State *L, const char *what)
{
    int top = lua_gettop(L);
    LOG_DEBUG("(%s: top = %d)", what, top);
    for (int depth = top; depth > 0; depth--) {
        int inv = depth - top - 1;
#define CUR L, depth
        switch (lua_type(CUR)) {
        case LUA_TNIL: // 0
            LOG_DEBUG("- %d(%d): LUA_TNIL", depth, inv);
            break;
        case LUA_TBOOLEAN: // 1
            LOG_DEBUG("- %d(%d): LUA_TBOOLEAN (%s)", depth, inv,
              lua_toboolean(CUR) ? "true" : "false");
            break;
        case LUA_TLIGHTUSERDATA: // 2
            LOG_DEBUG("- %d(%d): LUA_TLIGHTUSERDATA (p=%p)", depth, inv,
              lua_touserdata(CUR));
            break;
        case LUA_TNUMBER: // 3
            if (lua_isinteger(CUR)) {
                LOG_DEBUG("- %d(%d): LUA_TNUMBER (int %lld)", depth, inv,
                  (long long)lua_tointegerx(CUR, NULL));
            } else {
                LOG_DEBUG("- %d(%d): LUA_TNUMBER (num %f)", depth, inv,
                  lua_tonumberx(CUR, NULL));
            }
            break;
        case LUA_TSTRING: // 4
        {
            size_t len;
            const char *p = lua_tolstring(CUR, &len);
            LOG_DEBUG("- %d(%d): LUA_TSTRING ('%s', len=%zu)", depth, inv, p,
              len);
            break;
        }
        case LUA_TTABLE: // 5
            if (lua_rawlen(CUR)) {
                LOG_DEBUG("- %d(%d): LUA_TTABLE rawlen=%lld", depth, inv,
                  (long long)lua_rawlen(CUR));
            } else {
                LOG_DEBUG("- %d(%d): LUA_TTABLE (%p)", depth, inv,
                  lua_topointer(CUR));
            }
            break;
        case LUA_TFUNCTION: // 6
            if (lua_tocfunction(CUR)) {
                LOG_DEBUG("- %d(%d): LUA_TFUNCTION (C func=%p)", depth, inv,
                  lua_tocfunction(CUR));
            } else {
                LOG_DEBUG("- %d(%d): LUA_TFUNCTION (lua func %p)", depth, inv,
                  lua_topointer(CUR));
            }
            break;
        case LUA_TUSERDATA: // 7
            LOG_DEBUG("- %d(%d): LUA_TUSERDATA (%p)", depth, inv,
              lua_touserdata(CUR));
            break;
        case LUA_TTHREAD: // 8
            LOG_DEBUG("- %d(%d): LUA_TTHREAD (lua_State=%p)", depth, inv,
              lua_tothread(CUR));
            break;
        default:
            LOG_DEBUG("- %d(%d): (unknown type %d)", depth, inv, lua_type(CUR));
            break;
        }
#undef CUR
    }
}

// tentative, assuming 4K page
#define BUFFER_SIZE (4 << 10)
struct ctx {
    struct tus_delta_update_stream stream;
    uint8_t buf_src[BUFFER_SIZE];
    uint8_t buf_delta[BUFFER_SIZE];
    uint8_t buf_patched[BUFFER_SIZE];

    // tentative, may dynamically allocate
    uint8_t buf_suspend[sizeof(struct tus_delta_update_stream) + BUFFER_SIZE];
};

// note: resulting 'buf' will contain up to 'limit' bytes of data.
static int
copy_lua_string(lua_State *L, const char *key, char *buf, size_t limit,
  size_t *bytes, const char *fallback)
{
    int lua_type = lua_getfield(L, -1, key);
    if (bytes)
        *bytes = 0;
    if (lua_type != LUA_TSTRING) {
        if ((lua_type == LUA_TNIL) && fallback) {
            const size_t len = strnlen(fallback, limit);
            memcpy(buf, fallback, len);
            memset(buf + len, 0, limit - len);
            LOG_HIDDEN("fallback: %s='%s'", key, buf);
            lua_remove(L, -1);
            return 0;
        }
        LOG_ERROR("field_type of '%s' != LUA_TSTRING (%d)", key, lua_type);
        lua_remove(L, -1);
        return ENOENT;
    }
    size_t len;
    const char *value = luaL_checklstring(L, -1, &len);
    if (limit < len) {
        LOG_ERROR("%s: len(%s) cannot be > %zu", key, value, limit);
        return ENOMEM;
    }
    memcpy(buf, value, len);
    memset(buf + len, 0, limit - len);
    LOG_DEBUG("got: '%s'='%s'", key, buf);
    lua_remove(L, -1);
    if (bytes)
        *bytes = len;
    return 0;
}

static int
to_algorithm_id(const char *name)
{
    if (!name || !name[0]) {
        // default
        return TUS_DELTA_UPDATE_ALGORITHM_BSDIFF;
    }
    if (strncmp(name, "bsdiff", sizeof("bsdiff")) == 0) {
        return TUS_DELTA_UPDATE_ALGORITHM_BSDIFF;
    }
    LOG_ERROR("invalid algorithm '%s'", name);
    return -1;
}

static int
refill(lua_State *L, int config_index, const char *func_name, void *buffer,
  size_t limit, size_t offset, size_t *filled)
{
    int err = 0;
    const int top = lua_gettop(L);
    // dump_lua_stack(L, "pre");
    if (lua_type(L, config_index) != LUA_TTABLE) {
        LOG_ERROR("Lua stack #%d shall be a 'config' table, got %d",
          config_index, lua_type(L, config_index));
        return EINVAL;
    }
    int ltype = lua_getfield(L, config_index, func_name);
    if (ltype != LUA_TFUNCTION) {
        LOG_ERROR("missing '%s' in config table", func_name);
        lua_settop(L, top);
        return EINVAL;
    }
    if (!buffer) {
        LOG_ERROR("bad buffer for %s()", func_name);
        lua_settop(L, top);
        return EACCES;
    }
    LOG_DEBUG("calling %s(config, size=%zd, offset=%zd)", func_name, limit,
      offset);
    lua_pushvalue(L, 1); // use the table given from the caller as first arg.
    lua_pushinteger(L, limit);
    lua_pushinteger(L, offset);
    // dump_lua_stack(L, "pre pcall ");
    int lerr = lua_pcall(L, 3, 2, 0);
    if (lerr != LUA_OK) {
        LOG_ERROR("%s() -> lerr %d", func_name, lerr);
        LOG_ERROR("failed to generate initial delta: %s", lua_tostring(L, -1));
        lua_settop(L, top);
        return EINVAL; // ToDo: inherit error obj?
    }
    // expect blob or {nul, errobj}
    // there is no luaL_tolstringx(), have to check type explicitly to
    // distinguish 'nil'
    size_t len = 0;
    const char *value = NULL;
    if (lua_type(L, -2) == LUA_TSTRING) {
        value = luaL_tolstring(L, -2, &len);
        if (!value) {
            LOG_ERROR("bad string %d?", lua_type(L, -2));
            lua_settop(L, top);
            return EINVAL;
        }
    } else {
        // ToDO: the errobj shall be propagated to the caller
        LOG_ERROR("%s returnen and errobj?", func_name);
        lua_settop(L, top);
        return EINVAL;
    }
    if (limit < len) {
        LOG_ERROR("blob too large %zd", len);
        lua_settop(L, top);
        return ENOMEM;
    }
    memcpy(buffer, value, len);
    if (len < limit) {
        // callback may return smaller blob, ensure to wipe rest of buffer
        LOG_DEBUG("wipe [%zd -- %zd)", len, limit);
        memset((char *)buffer + len, 0, limit - len);
    }
    LOG_HIDDEN("got %zd bytes '%.8s...", len, (char *)buffer);
    lua_settop(L, top);
    if (filled)
        *filled = len;
    return err;
}

static void
debug_dump_stream(struct tus_delta_update_stream *stream)
{
    if (verbosity > 0) {
        LOG_DEBUG("- src.buf.pos %p = %zd/%zd @ %zd", stream->buf_src,
          (ssize_t)stream->src.buf.pos, (size_t)stream->src.buf.size,
          (size_t)stream->src.total_pos);
        LOG_DEBUG("- delta.buf.pos %p = %zd/%zd @ %zd", stream->buf_delta,
          (ssize_t)stream->delta.buf.pos, (size_t)stream->delta.buf.size,
          (size_t)stream->delta.total_pos);
        LOG_DEBUG("- patched.buf.pos %p = %zd/%zd @ %zd", stream->buf_patched,
          (ssize_t)stream->patched.buf.pos, (size_t)stream->patched.buf.size,
          (size_t)stream->patched.total_pos);
    }
}

// create a new stream object
static int
delta_update_initialize_stream(lua_State *L)
{
    int err;

    // initialize_stream()'s first arg shall be a config table
    const int config_index = 1;
    if (lua_type(L, config_index) != LUA_TTABLE) {
        LOG_ERROR("Lua stack #%d shall be a 'config' table, got %d",
          config_index, lua_type(L, config_index));
        return EINVAL;
    }

    // parameters are gives sa a table ('config')
    uint8_t algorithm_id = -1;
    {
        char value[16] = {};
        err = copy_lua_string(L, "algorithm", value, sizeof(value) - 1, NULL,
          "bsdiff");
        if (err)
            goto done;
        algorithm_id = to_algorithm_id(value);
        LOG_DEBUG("algorithm_id=%d:%s", algorithm_id, value);
    }

    struct ctx *ctx = lua_newuserdata(L, sizeof(*ctx));
    memset(ctx, 0, sizeof(*ctx)); // let Coverity know '*ctx' is clean

    const int ud_index = lua_gettop(L);

    { // init buf_delta at offset==0 to init stream
        size_t offset = 0;
        err = refill(L, config_index, "feed_delta", ctx->buf_delta,
          sizeof(ctx->buf_delta), offset, NULL);
    }

    // no need to init buf_patched[]

    // tus_delta_update_initialize() will init stream.buf_delta by ctx->buf_delta 
    // todo: check endianess?
    int derr = tus_delta_update_initialize(ctx->buf_delta,
      sizeof(ctx->buf_delta), sizeof(ctx->buf_src), sizeof(ctx->buf_patched),
      algorithm_id, &(ctx->stream));
    if (derr == TUS_DELTA_UPDATE_PATCH_OK) {
        if (ctx->stream.save_size < 0 ||
          (size_t)(ctx->stream.save_size) > sizeof(ctx->buf_suspend)) {
            LOG_DEBUG("save_size=%zd > %zd", (size_t)(ctx->stream.save_size),
              sizeof(ctx->buf_suspend));
            err = EINVAL;
        }
    } else {
        LOG_ERROR("tus_delta_update_initialize() failed %d", derr);
        err = EINVAL;
    }

    if (err) {
        (void)tus_delta_update_cancel(&(ctx->stream));
        (void)tus_delta_update_finalize(&(ctx->stream));
        goto done;
    }

    // set buf_patched before calling() patch to spare one call
    ctx->stream.buf_patched = ctx->buf_patched;
    // ctx->stream.buf_src is allowed to be initially NULL (may never needed)

    LOG_DEBUG("tus_delta_update_initialize() -> ctx=%p, save_size=%zd", ctx,
      (size_t)(ctx->stream.save_size));
    debug_dump_stream(&(ctx->stream));

    lua_settop(L, ud_index);
done:
    if (err) {
        lua_settop(L, config_index);
        lua_pushnil(L);
        lua_pushinteger(L, err);
        return 2; // return {nil, errobj}
    }
    // set meta-table for new 'userdata' and return it
    luaL_getmetatable(L, CONTEXT);
    lua_setmetatable(L, -2);
    return 1;
}

static int
stream_suspend(lua_State *L)
{
    // to be called as stream:suspend(), returns an blob or {nil, errobj}
    struct ctx *ctx = luaL_testudata(L, 1, CONTEXT);
    if (!ctx) {
        LOG_ERROR("invalid call %p", L);
        lua_pushnil(L);
        lua_pushinteger(L, EINVAL);
        return 2;
    }
    memset(ctx->buf_suspend, 0, sizeof(ctx->buf_suspend));
    // ToDo: would be better if size of buffer could be pass as a limiter
    int ret = tus_delta_update_suspend(&(ctx->stream), ctx->buf_suspend);
    if (ret == TUS_DELTA_UPDATE_PATCH_OK) {
        // return suspended state as a blob
        lua_pushlstring(L, (void *)ctx->buf_suspend, ctx->stream.save_size);
        return 1;
    } else {
        LOG_ERROR("tus_delta_update_suspend() failed %d", ret);
        lua_pushnil(L);
        lua_pushinteger(L, EINVAL);
        return 2;
    }
}

static int
validate_stream_sizes(const struct ctx *ctx)
{
    if (ctx->stream.patched.buf.size <= 0 ||
      sizeof(ctx->buf_patched) < (size_t)ctx->stream.patched.buf.size ||
      ctx->stream.patched.buf.pos > ctx->stream.patched.buf.size) {
        LOG_ERROR("unexpected resumed buf_patched state: size=%zd, pos=%zd",
          ctx->stream.patched.buf.size, ctx->stream.patched.buf.pos);
        return EINVAL;
    }
    if (ctx->stream.delta.buf.size <= 0 ||
      sizeof(ctx->buf_delta) < (size_t)ctx->stream.delta.buf.size ||
      ctx->stream.delta.buf.pos > ctx->stream.delta.buf.size) {
        LOG_ERROR("unexpected resumed buf_delta state: size=%zd, pos=%zd",
          ctx->stream.delta.buf.size, ctx->stream.delta.buf.pos);
        return EINVAL;
    }
    return 0;
}

static int
delta_update_resume_stream(lua_State *L)
{
    // to be called as delta_update.resume_stream(config, blob)
    // initialize_stream()'s first arg shall be a config table
    const int config_index = 1;
    if (lua_type(L, config_index) != LUA_TTABLE) {
        LOG_ERROR("Lua stack #%d shall be a 'config' table, got %d",
          config_index, lua_type(L, config_index));
        return EINVAL;
    }

    const int suspended_index = 2;
    if (lua_type(L, suspended_index) != LUA_TSTRING) {
        LOG_ERROR("Lua stack #%d shall be a suspended state, got %d",
          suspended_index, lua_type(L, suspended_index));
        lua_pushnil(L);
        lua_pushinteger(L, EINVAL);
        return 2;
    }
    size_t len = 0;
    struct ctx *ctx;
    const void *suspended = lua_tolstring(L, -1, &len);
    if (suspended && (len <= sizeof(ctx->buf_suspend))) {
        // create a new context and initialize it using 'suspended' blob
        ctx = lua_newuserdata(L, sizeof(*ctx));
        memset(ctx, 0, sizeof(*ctx)); // let Coverity know '*ctx' is clean
        memcpy(ctx->buf_suspend, suspended, len);

        // ToDo: shall be able to pass actual data size?
        int dret = tus_delta_update_resume(&(ctx->stream), ctx->buf_suspend);
        LOG_DEBUG("tus_delta_update_resume() -> %d", dret);

        if (dret == TUS_DELTA_UPDATE_PATCH_OK) {
            int err;
            err = validate_stream_sizes(ctx);
            if (err) {
                lua_settop(L, suspended_index);
                lua_pushnil(L);
                lua_pushinteger(L, err);
                return 2; // return {nil, err}
            }

            // tus_delta_update_resume() taints ctx->stream.buf_patched, resets
            // buf_src/buf_delta to NULL

            // set buf_delta[]
            size_t rem;
            rem = ctx->stream.delta.total_pos % ctx->stream.delta.buf.size;
            LOG_DEBUG("(ctx->stream.delta.total_pos = %zd, rem %zd)",
              ctx->stream.delta.total_pos, rem);
            ctx->stream.buf_delta = ctx->buf_delta;
            err = refill(L, config_index, "feed_delta", ctx->stream.buf_delta,
              ctx->stream.delta.buf.size, ctx->stream.delta.total_pos - rem,
              NULL);
            if (err) {
                LOG_ERROR("failed to set buf_delta[] %d", err);
                lua_settop(L, suspended_index);
                lua_pushnil(L);
                lua_pushinteger(L, err);
                return 2; // return {nil, err}
            }

            // set buf_src[]
            rem = ctx->stream.src.total_pos % ctx->stream.src.buf.size;
            LOG_DEBUG("(ctx->stream.src.total_pos = %zd, rem %zd)",
              ctx->stream.src.total_pos, rem);
            ctx->stream.buf_src = ctx->buf_src;
            err = refill(L, config_index, "feed_src", ctx->stream.buf_src,
              ctx->stream.src.buf.size, ctx->stream.src.total_pos - rem, NULL);
            if (err) {
                LOG_ERROR("failed to set buf_src[] %d", err);
                lua_settop(L, suspended_index);
                lua_pushnil(L);
                lua_pushinteger(L, err);
                return 2; // return {nil, err}
            }

            // coerce to use buf_patched[] (not a part of buf_suspend[]) for
            // patched data
            memcpy(ctx->buf_patched, ctx->stream.buf_patched,
              ctx->stream.patched.buf.size);
            ctx->stream.buf_patched = ctx->buf_patched;

            // set meta-method for the new userdata
            luaL_getmetatable(L, CONTEXT);
            lua_setmetatable(L, -2);

            return 1; // resumed, return userdata

        } else if (dret == TUS_DELTA_UPDATE_PATCH_SKIP) {
            int err;
            err = validate_stream_sizes(ctx);
            if (err) {
                lua_settop(L, suspended_index);
                lua_pushnil(L);
                lua_pushinteger(L, err);
                return 2; // return {nil, err}
            }
            // set buf_delta[]
            size_t rem;
            rem = ctx->stream.delta.total_pos % ctx->stream.delta.buf.size;
            LOG_DEBUG("(ctx->stream.delta.total_pos = %zd, rem %zd)",
              ctx->stream.delta.total_pos, rem);
            ctx->stream.buf_delta = ctx->buf_delta;
            err = refill(L, config_index, "feed_delta", ctx->stream.buf_delta,
              ctx->stream.delta.buf.size, ctx->stream.delta.total_pos - rem,
              NULL);
            if (err) {
                LOG_ERROR("failed to set buf_delta[] %d", err);
                lua_settop(L, suspended_index);
                lua_pushnil(L);
                lua_pushinteger(L, err);
                return 2; // return {nil, err}
            }

            // coerce to use buf_patched[] (not a part of buf_suspend[]) for
            // patched data
            memcpy(ctx->buf_patched, ctx->stream.buf_patched,
              ctx->stream.patched.buf.size);
            ctx->stream.buf_patched = ctx->buf_patched;

            // set meta-method for the new userdata
            luaL_getmetatable(L, CONTEXT);
            lua_setmetatable(L, -2);

            return 1; // resumed, return userdata
        } else {
            LOG_ERROR("failed to resume(): %d", dret);
            lua_settop(L, suspended_index);
            lua_pushnil(L);
            lua_pushinteger(L, EINVAL);
            return 2; // return {nil, err}
        }
    } else {
        LOG_ERROR("invalid resume info, %p, %zd", suspended, len);
        lua_settop(L, suspended_index);
        lua_pushnil(L);
        lua_pushinteger(L, EINVAL);
        return 2; // return {nil, err}
    }
}

static int
stream_finalize(lua_State *L)
{
    struct ctx *ctx = luaL_testudata(L, 1, CONTEXT);
    if (!ctx) {
        LOG_ERROR("invalid call %p", L);
        lua_pushnil(L);
        lua_pushinteger(L, EINVAL);
        return 2;
    }
    if (ctx->stream.state >= TUS_DELTA_UPDATE_STATE__LIMIT) {
        LOG_ERROR("corrupted state? %d", ctx->stream.state);
        return 0;
    }
    switch (ctx->stream.state) {
    default:
        LOG_DEBUG("%p: ctx=%p, ctx->stream.state=%d (cancel now)", L, ctx,
          ctx->stream.state);
        (void)tus_delta_update_cancel(&(ctx->stream));
	/* fallthrough */
    case TUS_DELTA_UPDATE_STATE_CANCELED:
        LOG_DEBUG("%p: ctx=%p, ctx->stream.state=%d (finalize now)", L, ctx,
          ctx->stream.state);
        (void)tus_delta_update_finalize(&(ctx->stream));
        break;
    case TUS_DELTA_UPDATE_STATE_FINISHED:
        LOG_DEBUG("%p: ctx=%p, ctx->stream.state=%d (already finalized)", L,
          ctx, ctx->stream.state);
        break;
    case TUS_DELTA_UPDATE_STATE_SUSPENDED:
        // fixme: cannot call tus_delta_update_finalize() here> (seems to
        // trigger warnings)
        LOG_DEBUG("%p: ctx=%p, ctx->stream.state=%d (already suspended)", L,
          ctx, ctx->stream.state);
        break;
    }
    return 0;
}

static int
stream_patch(lua_State *L)
{
    // stream:patch(), returns state or nil,errobj
    struct ctx *ctx = luaL_testudata(L, 1, CONTEXT);
    if (!ctx) {
        LOG_ERROR("invalid call %p", L);
        lua_pushnil(L);
        lua_pushinteger(L, EINVAL);
        return 2;
    }
    LOG_DEBUG("%p: pre ctx=%p, ctx->stream.state=%d", L, ctx,
      ctx->stream.state);
    debug_dump_stream(&(ctx->stream));

    int ret = tus_delta_update_patch(&(ctx->stream));
    LOG_DEBUG(" => ret=%d, ctx->stream.state=%d", ret, ctx->stream.state);
    debug_dump_stream(&(ctx->stream));

    lua_pushinteger(L, ret);
    return 1;
}

static int
stream_skip(lua_State *L)
{
    // stream:skip(config, offset), returns result or nil, errobj
    struct ctx *ctx = luaL_testudata(L, 1, CONTEXT);
    if (!ctx) {
        LOG_ERROR("invalid call %p", L);
        lua_pushnil(L);
        lua_pushinteger(L, EINVAL);
        return 2;
    }

    int is_int = 0;
    lua_Unsigned offset = lua_tointegerx(L, -1, &is_int);
    if (!is_int) {
        LOG_ERROR("expected an integer (%d)", lua_type(L, -1));
        lua_pushnil(L);
        lua_pushinteger(L, EINVAL);
        return 1;
    }

    LOG_DEBUG("%p: pre ctx=%p, ctx->stream.state=%d, offset=%zd", L, ctx,
      ctx->stream.state, (size_t)offset);

    const int ret = tus_delta_update_skip(&(ctx->stream), offset);
    LOG_DEBUG(" => ret=%d, ctx->stream.state=%d", ret, ctx->stream.state);

    debug_dump_stream(&(ctx->stream));

    lua_pushinteger(L, ret);
    return 1;
}

static int
stream_cancel(lua_State *L)
{
    // stream:cancel(), returns nil or errobj
    struct ctx *ctx = luaL_testudata(L, 1, CONTEXT);
    if (!ctx) {
        LOG_ERROR("invalid call %p", L);
        lua_pushinteger(L, EINVAL);
        return 1;
    }
    LOG_DEBUG("%p: ctx=%p", L, ctx);

    int ret = tus_delta_update_cancel(&(ctx->stream));
    if (ret != TUS_DELTA_UPDATE_PATCH_OK) {
        lua_pushinteger(L, ret);
        return 1;
    }
    return 0;
}

static int
stream_get_patched_size(lua_State *L)
{
    // stream:get_patched_size(), returns int or nil,errobj
    struct ctx *ctx = luaL_testudata(L, 1, CONTEXT);
    if (!ctx) {
        LOG_ERROR("invalid call %p", L);
        lua_pushnil(L);
        lua_pushinteger(L, EINVAL);
        return 2;
    }
    LOG_DEBUG("%p: ctx=%p, ctx->stream.total_patched_size=%zd", L, ctx,
      (size_t)ctx->stream.total_patched_size);

    lua_pushinteger(L, ctx->stream.total_patched_size);
    return 1;
}

static int
stream_get_patched_offset(lua_State *L)
{
    // stream:get_patched_offset(), returns int or nil,errobj
    struct ctx *ctx = luaL_testudata(L, 1, CONTEXT);
    if (!ctx) {
        LOG_ERROR("invalid call %p", L);
        lua_pushnil(L);
        lua_pushinteger(L, EINVAL);
        return 2;
    }
    LOG_DEBUG("%p: ctx=%p, ctx->stream.patched.total_pos=%zd", L, ctx,
      (size_t)ctx->stream.patched.total_pos);

    lua_pushinteger(L, ctx->stream.patched.total_pos);
    return 1;
}

static int
stream_get_patched_blob(lua_State *L)
{
    // stream:get_patched_blob(), returns blob(Lua string) or nil,errobj
    struct ctx *ctx = luaL_testudata(L, 1, CONTEXT);
    if (!ctx) {
        LOG_ERROR("invalid call %p", L);
        lua_pushnil(L);
        lua_pushinteger(L, EINVAL);
        return 2;
    }
    LOG_DEBUG("%p: ctx=%p, ctx->stream.patched.buf.pos=%zd", L, ctx,
      (size_t)ctx->stream.patched.buf.pos);

    lua_pushlstring(L, (const char *)ctx->stream.buf_patched,
      ctx->stream.patched.buf.pos);
    // re-use ctx->stream.buf_patched
    ctx->stream.patched.buf.pos = 0;
    return 1;
}

static int
stream_get_state(lua_State *L)
{
    // stream:get_patched_size(), returns int or nil,errobj
    struct ctx *ctx = luaL_testudata(L, 1, CONTEXT);
    if (!ctx) {
        LOG_ERROR("invalid call %p", L);
        lua_pushnil(L);
        lua_pushinteger(L, EINVAL);
        return 2;
    }
    LOG_DEBUG("%p: ctx=%p, ctx->stream.state=%d", L, ctx, ctx->stream.state);
    lua_pushinteger(L, ctx->stream.state);
    return 1;
}

static int
stream_set_delta(lua_State *L)
{
    // stream:set_delta(config.feed_delta, config), returns nil or errobj

    struct ctx *ctx = luaL_testudata(L, 1, CONTEXT);
    if (!ctx) {
        LOG_ERROR("invalid call %p", L);
        lua_pushinteger(L, EINVAL);
        return 1;
    }
    LOG_DEBUG("%p: ctx=%p, ctx->stream.delta.total_pos=%zd", L, ctx,
      (size_t)ctx->stream.delta.total_pos);

    debug_dump_stream(&(ctx->stream));
    if (ctx->stream.delta.buf.size > 0) {
        size_t rem = ctx->stream.delta.total_pos % ctx->stream.delta.buf.size;
        // re-use ctx->stream.buf_delta
        int err = refill(L, -1, "feed_delta", ctx->stream.buf_delta,
          ctx->stream.delta.buf.size, ctx->stream.delta.total_pos - rem, NULL);
        if (!err) {
            ctx->stream.delta.buf.pos = rem;
        } else {
            lua_pushinteger(L, err);
            return 1;
        }
    } else {
        ctx->stream.delta.buf.pos = 0;
    }
    return 0;
}

static int
stream_set_src(lua_State *L)
{
    // stream:set_src(config.feed_delta, config), returns nil or errobj

    struct ctx *ctx = luaL_testudata(L, 1, CONTEXT);
    if (!ctx) {
        LOG_ERROR("invalid call %p", L);
        lua_pushinteger(L, EINVAL);
        return 1;
    }
    LOG_DEBUG("%p: ctx=%p, ctx->stream.src.total_pos=%zd", L, ctx,
      (size_t)ctx->stream.src.total_pos);

    debug_dump_stream(&(ctx->stream));
    if (ctx->stream.src.buf.size > 0) {
        if ((size_t)ctx->stream.src.buf.size > sizeof(ctx->buf_src)) {
            LOG_ERROR("invalid request size %zd",
              (size_t)ctx->stream.src.buf.size);
            lua_pushinteger(L, EINVAL);
            return 1;
        }
        size_t rem = ctx->stream.src.total_pos % ctx->stream.src.buf.size;
        // re-use ctx->stream.buf_src
        int err = refill(L, -1, "feed_src", ctx->buf_src,
          ctx->stream.src.buf.size, ctx->stream.src.total_pos - rem, NULL);
        if (!err) {
            ctx->stream.buf_src = ctx->buf_src;
            ctx->stream.src.buf.pos = rem;
        } else {
            lua_pushinteger(L, err);
            return 1;
        }
    } else {
        ctx->stream.src.buf.pos = 0;
    }
    return 0;
}

static void
init_stream_metatable(lua_State *L)
{
    LOG_HIDDEN("%p\n", L);

    static const struct luaL_Reg metamethods[] = { { "__index",
                                                     NULL }, // placeholder
        //	{"__gc", stream_finalize},
        { NULL, NULL } };

    // add a meta-table to the Lua registry
    luaL_newmetatable(L, CONTEXT);
    luaL_setfuncs(L, metamethods, 0);

    static const struct luaL_Reg methods[] = { { "cancel", stream_cancel },
        { "finalize", stream_finalize },
        { "get_patched_blob", stream_get_patched_blob },
        { "get_patched_offset", stream_get_patched_offset },
        { "get_patched_size", stream_get_patched_size },
        { "get_state", stream_get_state }, { "patch", stream_patch },
        { "set_delta", stream_set_delta }, { "set_src", stream_set_src },
        { "skip", stream_skip }, { "suspend", stream_suspend },
        { NULL, NULL } };
    luaL_newlibtable(L, methods); // method table for stream objs
    luaL_setfuncs(L, methods, 0);

    lua_setfield(L, -2,
      "__index"); // set the method table as metatable.__index */
    lua_pop(L, 1);

    // dump_lua_stack(L);
    LOG_DEBUG("%p: done", L);
}

int
luaopen_tmc_delta_update(lua_State *L)
{

    LOG_HIDDEN("%p\n", L);
    init_stream_metatable(L);

    static const struct luaL_Reg lib[] = { { "initialize_stream",
                                             delta_update_initialize_stream },
        { "resume_stream", delta_update_resume_stream }, { NULL, NULL } };
    luaL_newlib(L, lib);

    LOG_DEBUG("%p: done", L);
    // dump_lua_stack(L, "done");

    return 1;
}
