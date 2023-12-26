/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */

#include <assert.h>
#include <errno.h>
#include <lauxlib.h>
#include <limits.h>
#include <lua.h>
#include <lualib.h>
#include <pthread.h>
#include <queue.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "logger.h"
#include "runner_dc_grpc.h"

// for LUA_REGISTRYINDEX
static char lua_registry_key[] = "uo_ctx";
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

struct update_event {
    SIMPLEQ_ENTRY(update_event) next;
    // ToDo: prepare dedicated pages for entries
    char *key;
    char *value;
    const char *domain_id;
};

struct async_op_state {
    TAILQ_ENTRY(async_op_state) next;
    int domain;
    uint32_t tag;
    int result_code;
    void *result;
    size_t size;
};

#define MAX_DC (4) // tentative
struct ctx {
    pthread_mutex_t mtx;
    pthread_cond_t cv;

    char session_id[16]; // ToDo: keep an uuid for current session?

    SIMPLEQ_HEAD(, update_event) queued_updates;

    lua_State *L;
    int lua_refs;

    struct grpc_worker {
        struct ctx *ctx;
        char name[NAME_MAX]; // FIXME: tentative
        char target[NAME_MAX];
        pthread_t th;
        enum state {
            WORKER_STATE_NONE = 0,
            WORKER_STATE_RUNNING,
            WORKER_STATE_SHUTDOWN,
        } state,
          req;
        int error;
        void *arg;
        int connected; // connection health
    } grpc[MAX_DC];
    int num_grpc_workers;

    // url of a UO Lua script (to be seved from embeddeed HTTPd)
    const char *script_url;
    // local / HTTPd data dir prefix (== campaign ID)
    const char *prefix;

    TAILQ_HEAD(, async_op_state) async_done;

    int debug;

    int initialized_curl;
};

static int
init_pthread(struct ctx *ctx)
{
    int err;

    err = pthread_mutex_init(&(ctx->mtx), NULL);
    if (!err) {
        err = pthread_cond_init(&(ctx->cv), NULL);
    }
    return (err);
}

static void
fini_pthread(struct ctx *ctx)
{

    pthread_cond_destroy(&(ctx->cv));
    pthread_mutex_destroy(&(ctx->mtx));
}

static int
lock_ctx(struct ctx *ctx)
{
    // LOG_DEBUG("%p locking %p", __builtin_return_address(0), &(ctx->mtx));
    int err = pthread_mutex_lock(&(ctx->mtx));
    return err;
}

static void
unlock_ctx(struct ctx *ctx)
{
    (void)pthread_mutex_unlock(&(ctx->mtx));
    //    LOG_DEBUG("%p UNlocked %p", __builtin_return_address(0), &(ctx->mtx));
}

static int
wait_ctx(struct ctx *ctx)
{
    // to be called while &(ctx->mtx) was locked
    return pthread_cond_wait(&(ctx->cv), &(ctx->mtx));
}

static int
wakeup_ctx(struct ctx *ctx)
{
    // to be called while &(ctx->mtx) was locked
    return pthread_cond_broadcast(&(ctx->cv));
}

static void
dump_lua_error(lua_State *L, const char *errname)
{
    const char *errmsg = lua_tostring(L, -1);
    LOG_ERROR("Lua error[%s]:\n----\n"
              "%s"
              "\n----",
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

static int process_dc_fsm_update(lua_State *L, const char *domain_id,
  const char *key, const char *value);

static int release_lua_state_locked(struct ctx *ctx, lua_State *L);

static int
get_events_from_lua(lua_State *L)
{
    struct ctx *ctx = get_ctx_from_lua(L);
    if (ctx && lock_ctx(ctx) == 0) {
        LOG_DEBUG("wait for events (%p)", ctx);

        while (!SIMPLEQ_EMPTY(&(ctx->queued_updates))) {
            int applied = 0;
            struct update_event *e;
            while ((e = SIMPLEQ_FIRST(&(ctx->queued_updates))) != NULL) {
                SIMPLEQ_REMOVE_HEAD(&(ctx->queued_updates), next);
                int ret = process_dc_fsm_update(L, e->domain_id, e->key,
                  e->value);
                if (ret) {
                    LOG_ERROR("failed to process an update event (%d)", ret);
                } else {
                    applied++;
                }
                free(e);
            }
            if (applied > 0) {
                // did some updates, kick transitions
                wakeup_ctx(ctx);
                unlock_ctx(ctx);
                return 0;
            }
            release_lua_state_locked(ctx, L);
        }

        wait_ctx(ctx); // someone will wake me up

        LOG_DEBUG("woken (%p)", ctx);
        unlock_ctx(ctx);
    }
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
            if (TAILQ_EMPTY(&(ctx->async_done))) {
                if (nonblock) {
                    // no 'done' in queue, force to return an empty table
                    break;
                }
                do {
                    wait_ctx(ctx);
                } while (TAILQ_EMPTY(&(ctx->async_done)));
            } else {
                // some 'done' events has been queued.
                // shall return a tag if matching one could be found,
                // but inihibit looping
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
                    TAILQ_FOREACH (p, &(ctx->async_done), next) {
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
            LOG_ERROR("got non-tag %u?", (uint32_t)atag);
            ret = EINVAL;
        }
        if (!ret) {
            lua_pop(L, 1);
            LOG_DEBUG("- arg = %u", (uint32_t)atag);

            struct async_op_state *p = NULL;
            TAILQ_FOREACH (p, &(ctx->async_done), next) {
                if (p->tag == atag)
                    break;
            }
            if (p) {
                LOG_DEBUG("- %p", p);
                TAILQ_REMOVE(&(ctx->async_done), p, next);
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

static int init_grpc_client(struct grpc_worker *wk);

static int
start_grpc_worker(lua_State *L)
{
    int ret = 0;

    struct ctx *ctx = get_ctx_from_lua(L);
    if (lock_ctx(ctx) == 0) {
        if (ctx->num_grpc_workers >= MAX_DC) {
            ret = ENOMEM;
        } else {
            const char *domain_id = lua_tostring(L, 1);
            const char *target = lua_tostring(L, 2);
            for (int i = 0; i < ctx->num_grpc_workers; i++) {
                if (strncmp(ctx->grpc[i].name, domain_id,
                      sizeof(ctx->grpc[i].name)) == 0) {
                    LOG_ERROR("'%s' already runnning", domain_id);
                    ret = EINVAL;
                }
            }
            if (!ret) {
                struct grpc_worker *wk = ctx->grpc + ctx->num_grpc_workers;
                wk->ctx = ctx;
                strncpy(wk->name, domain_id, sizeof(wk->name) - 1);
                strncpy(wk->target, target, sizeof(wk->target) - 1);
                wk->connected = 0; // not yet connected
                ctx->num_grpc_workers++;
                unlock_ctx(ctx);
                init_grpc_client(wk);
                LOG_DEBUG("domain '%s': gRPC '%s'", domain_id, target);
            } else {
                unlock_ctx(ctx);
            }
        }
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
init_lua(struct ctx *ctx)
{
    int err;
    lua_State *L = luaL_newstate();
    if (!(L)) {
        return ENOMEM;
    }
    LOG_DEBUG("created new Lua state(%p)", L);

    // init env
    luaL_openlibs(L); // enable libraries. fixme: apply some restrictions?

    /* registry[dc_ctx] = ctx */
    lua_pushlightuserdata(L, lua_registry_key);
    lua_pushlightuserdata(L, ctx);
    lua_settable(L, LUA_REGISTRYINDEX);

    err = dostring(L, "uo = require('uo')");
    if (err)
        return err;
    err = dostring(L, "tup = require('tup_parser')");
    if (err)
        return err;

    // additional members of 'uo' for UO script runtime
    err = dostring(L, "uo.fsm = require('uo_fsm');uo.fsm.uo=uo;");
    if (err)
        return err;
    err = dostring(L, "uo.async = require('async');uo.async.parent=uo;");
    if (err)
        return err;

    // setup 'uo' members
    lua_getglobal(L, "uo");
    {
        lua_getfield(L, -1, "async");
        lua_pushcfunction(L, wait_from_lua);
        lua_setfield(L, -2, "wait_from_lua");
        lua_pushcfunction(L, drop_from_lua);
        lua_setfield(L, -2, "drop_from_lua");
    }

    lua_settop(L, 0);

    err = dostring(L, "tup = require('tup_parser')");
    if (err)
        return err;
    if (ctx->debug > 0) {
        dostring(L, "uo.log('starting UO Lua script env', uo)");
        dostring(L, "uo.log('- P.path', package.path)");
        dostring(L, "uo.log('- P.cpath', package.cpath)");
    }
    if (err)
        return err;

    // LOG_DEBUG("lua_gettop() -> %d"\\, lua_gettop(L));

    if (!lua_pushstring(L, ctx->prefix)) {
        return EINVAL;
    }
    lua_setglobal(L, "PREFIX");
    err = dostring(L, "uo.log('- prefix: ', PREFIX)");
    if (err)
        return err;

    assert(lua_gettop(L) == 0);

    // update module 'uo'
    lua_getglobal(L, "uo"); // stack[1]

    lua_pushcfunction(L, get_events_from_lua);
    lua_setfield(L, 1, "get_events_from_lua");
    dostring(L, "uo.log('- uo.get_events_from_lua', uo.get_events_from_lua)");

    lua_pushstring(L, ctx->session_id);
    lua_setfield(L, 1, "session_id");
    dostring(L, "uo.log('- uo.session_id', uo.session_id)");

    lua_pushcfunction(L, start_grpc_worker);
    lua_setfield(L, 1, "start_grpc_worker");
    dostring(L, "uo.log('- uo.start_grpc_worker', start_grpc_worker)");

    dostring(L, "uo:build_domains(true)");

    lua_settop(L, 0);
    ctx->L = L;
    return 0;
}

static lua_State *
grab_lua_state_locked(struct ctx *ctx)
{
    lua_State *L = NULL;

    while (ctx->lua_refs > 0) {
        wait_ctx(ctx);
        // ToDo: trap emergency events
    }

    // ToDo: support recursive lock / owner-checker?
    L = ctx->L;
    ctx->lua_refs++;

    LOG_DEBUG("(grabbed Lua ref of %p)", L);

    return L;
}

static lua_State *
grab_lua_state(struct ctx *ctx)
{
    lua_State *L = NULL;
    if (lock_ctx(ctx) == 0) {
        L = grab_lua_state_locked(ctx);
        unlock_ctx(ctx);
    }
    return L;
}

static int
release_lua_state_locked(struct ctx *ctx, lua_State *L)
{
    int ret = EINVAL;

    if (ctx->lua_refs > 0 && ctx->L == L) {
        ctx->lua_refs--;
        LOG_DEBUG("(decreased Lua ref of %p to %d)", L, ctx->lua_refs);
        wakeup_ctx(ctx);
        ret = 0;
    }
    return ret;
}

static int
release_lua_state(struct ctx *ctx, lua_State *L)
{
    int ret = EINVAL;

    if (lock_ctx(ctx) == 0) {
        ret = release_lua_state_locked(ctx, L);
        unlock_ctx(ctx);
    }
    return ret;
}

static void
fini_lua(struct ctx *ctx)
{
    if (lock_ctx(ctx) == 0) {
        lua_State *L = ctx->L;
        if (L) {
            ctx->L = NULL;
            unlock_ctx(ctx);

            LOG_DEBUG("closing Lua state(%p)", L);

            // todo: flush pending Lua ops
            lua_close(L);
        }
    }
}

static int
handle_fsm_env(struct ctx *ctx, const char *k, const char *v,
  const char *domain_id)
{
    size_t k_len = strnlen(k, PATH_MAX);
    if (k_len >= PATH_MAX) {
        return EINVAL;
    }
    size_t v_len = v ? strnlen(v, PATH_MAX) : 0;
    if (v_len >= PATH_MAX) {
        return EINVAL;
    }

    struct update_event *e;
    size_t total_size = sizeof(*e) + k_len + 1 + v_len + 1;
    void *p = malloc(total_size);
    if (!p)
        return (ENOMEM);
    memset(p, 0, total_size);

    e = p;
    e->key = (char *)p + sizeof(*e);
    e->domain_id = domain_id;
    memcpy(e->key, k, k_len);

    if (v) {
        e->value = e->key + k_len + 1;
        memcpy(e->value, v, v_len);
        LOG_DEBUG(" - %s: '%s' of domain '%s' is now '%s'", ctx->session_id, k,
          domain_id, v);
    } else {
        e->value = NULL;
        LOG_DEBUG(" - %s: '%s' of domain '%s' is now NULL", ctx->session_id, k,
          domain_id);
    }

    lock_ctx(ctx);
    SIMPLEQ_INSERT_TAIL(&(ctx->queued_updates), e, next);
    // LOG_DEBUG("- enqueue %p %s %s\n", e, e->key, e->value);

    wakeup_ctx(ctx);
    unlock_ctx(ctx);

    return 0;
}

static int
handle_async_op(struct ctx *ctx, uint32_t tag, int result_code, void *result,
  size_t size)
{
    LOG_DEBUG(" - got tag %d -> code %d", tag, result_code);

    struct async_op_state *ao;
    void *p = malloc(sizeof(*ao));
    if (!p)
        return (ENOMEM);
    memset(p, 0, sizeof(*ao));

    ao = p;
    ao->tag = tag;
    ao->result_code = result_code;
    (void)result;
    (void)size;
    //    ao->result = ...

    lock_ctx(ctx);
    // ToDo; ordering?
    TAILQ_INSERT_TAIL(&(ctx->async_done), ao, next);
    // LOG_DEBUG("- enqueue %p %s %s\n", e, e->key, e->value);

    wakeup_ctx(ctx);
    unlock_ctx(ctx);

    return 0;
}

static void *
grpc_client(void *arg)
{
    struct grpc_worker *wk = (struct grpc_worker *)arg;
    struct ctx *ctx = wk->ctx;

    wk->error = lock_ctx(ctx);
    if (wk->error) {
        return NULL;
    }
    LOG_DEBUG("started gRPC worker for DC '%s' (%s)", wk->name, wk->target);

    wk->state = WORKER_STATE_RUNNING;
    wk->error = wakeup_ctx(ctx);
    if (wk->error) {
        unlock_ctx(ctx);
        return NULL;
    }
    struct grpc_domaincontroller_stub *stub = grpc_domaincontroller_new(
      wk->target);
    if (!stub) {
        unlock_ctx(ctx);
        return NULL;
    }
    LOG_DEBUG(" gRPC stub=%p", stub);
    while (wk->req == WORKER_STATE_RUNNING) {
        unlock_ctx(ctx);
        if (stub) {
            int err = grpc_domaincontroller_poll_update(ctx, stub,
              ctx->session_id, wk->name, &(wk->connected), handle_fsm_env,
              handle_async_op);
            if (err) {
                LOG_DEBUG("err = %d", err);
            } else {
                LOG_DEBUG("polled DC FSM event (for session '%s')",
                  ctx->session_id);
            }
        }
        lock_ctx(ctx);
        sleep(1);
    }
    LOG_DEBUG("leaving, req='%d'", wk->req);
    if (stub) {
        grpc_domaincontroller_delete(stub);
        stub = NULL;
    }

    wk->state = WORKER_STATE_NONE;
    (void)wakeup_ctx(ctx);
    unlock_ctx(ctx);
    return NULL;
}

static int
init_grpc_client(struct grpc_worker *wk)
{
    int err;
    struct ctx *ctx = wk->ctx;
    const pthread_attr_t *attr = NULL;

    LOG_DEBUG("starting worker '%s' %p", wk->name, ctx);
    err = lock_ctx(ctx);
    if (err == 0) {
        wk->req = WORKER_STATE_RUNNING;
        err = pthread_create(&(wk->th), attr, grpc_client, wk);
        if (!err) {
            while (wk->state == WORKER_STATE_NONE) {
                err = wait_ctx(ctx);
                if (err)
                    break;
            }
        } else {
            // failed to start
        }
        unlock_ctx(ctx);
    }
    return err;
}

static void
fini_grpc_client(struct ctx *ctx)
{
    int err;
    for (int i = ctx->num_grpc_workers - 1; i >= 0; i--) {
        struct grpc_worker *wk = &(ctx->grpc[i]);
        err = lock_ctx(ctx);
        if (err == 0) {
            while (wk->state == WORKER_STATE_RUNNING) {
                LOG_DEBUG("requesting to stop worker '%s'", wk->name);
                wk->req = WORKER_STATE_SHUTDOWN;
                err = wakeup_ctx(ctx);
                if (!err)
                    err = wait_ctx(ctx);
                if (err)
                    break;
            }
            LOG_DEBUG("join worker '%s'", wk->name);
            (void)pthread_join(wk->th, NULL);
            unlock_ctx(ctx);
        }
    }
}

static void
fini_ctx(struct ctx *ctx)
{
    LOG_DEBUG("finalizing ctx(%p)", ctx);
    fini_grpc_client(ctx);
    fini_lua(ctx);
    fini_pthread(ctx);
    LOG_DEBUG("finalized ctx(%p)", ctx);
}

//// todo: unify with src/target/dc/fetch.c
#include <curl/curl.h>
struct to_buffer {
    void *buffer;
    size_t base;
    size_t limit;
};
static size_t
write_buffer_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    struct to_buffer *dst = (struct to_buffer *)userdata;

    const size_t realsize = size * nmemb;
    LOG_HIDDEN("to=%p: %zu @ %zd\n", dst->buffer, realsize, dst->base);
    assert(realsize + dst->base >= dst->base);
    if (realsize + dst->base > dst->limit)
        return CURLE_WRITE_ERROR;

    memcpy((char *)(dst->buffer) + dst->base, ptr, realsize);
    LOG_CRITICAL("'%s'\n", (char *)dst->buffer + dst->base);
    dst->base += realsize;
    return realsize;
}
static int
fetch_to_buffer(CURL *c, const char *url, struct to_buffer *dst)
{
    CURLcode ret;

    ret = curl_easy_setopt(c, CURLOPT_URL, url);
    if (ret != CURLE_OK)
        return -1;

    // ToDo: set CURLOPT_HEADERFUNCTION ?

    ret = curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, write_buffer_callback);
    if (ret != CURLE_OK)
        return -1;
    ret = curl_easy_setopt(c, CURLOPT_WRITEDATA, dst);
    if (ret != CURLE_OK)
        return -1;
    ret = curl_easy_perform(c);
    if (ret != CURLE_OK)
        return -1;

    return 0;
}
static int
http_fetch_to_buffer(const char *url, void *buffer, size_t limit)
{
    int ret;
    CURL *c = curl_easy_init();
    if (c) {
        struct to_buffer dst = { .buffer = buffer, .base = 0, .limit = limit };
        ret = fetch_to_buffer(c, url, &dst);
        curl_easy_cleanup(c);
    } else {
        ret = EINVAL;
    }
    return ret;
}
////

static int
start_uo_script(struct ctx *ctx)
{
    int lerr;

    lua_State *L = ctx->L;

    // tentative, use HEAD to tell actual size?
    size_t limit = 4 * sysconf(_SC_PAGESIZE);
    void *buf = malloc(limit);
    if (!buf) {
        LOG_ERROR("malloc(%zd) failed", limit);
        return -1; // ENOMEM
    } else {
        CURLcode cerr;
        if (!ctx->initialized_curl) {
            cerr = curl_global_init(CURL_GLOBAL_DEFAULT);
            if (cerr != CURLE_OK) {
                LOG_ERROR("faield to initialized CURL %d\n", cerr);
                goto fail;
            }
            ctx->initialized_curl = 1;
        }

        memset(buf, 0, limit);
        if (http_fetch_to_buffer(ctx->script_url, buf, limit) != 0) {
            goto fail;
        }
    }

    // todo: validate the script?
    luaL_loadstring(L, buf);
    free(buf);
    // execute, i.e. populate global symbol tables (not starting main())
    lerr = lua_pcall(L, 0, 1, 0);
    if (lerr != LUA_OK) {
        dump_lua_error(L, ctx->script_url);
        // keep Lua state, which shall be released at fini_ctx() -> fini_lua()
        return -1;
    }

    // script may return an errir object
    switch (lua_type(L, -1)) {
    case LUA_TNIL:
        return 0;

    default:
        LOG_ERROR("UO script returned an error object type %d",
          lua_type(L, -1));
        return -1;
    }
fail:
    free(buf);
    return -1;
}

static int
init_ctx(struct ctx *ctx, const char *session_id)
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

    if (session_id) {
        memset(ctx->session_id, 0, sizeof(ctx->session_id));
        strncpy(ctx->session_id, session_id, sizeof(ctx->session_id) - 1);
    }
    SIMPLEQ_INIT(&(ctx->queued_updates));
    TAILQ_INIT(&(ctx->async_done));

    err = init_lua(ctx);
    if (err)
        return err;
#if 0
  err = init_monitor(ctx);
  if (err)
    return err;
#endif

    LOG_DEBUG("initialized ctx %p", ctx);

    return (err);
}

static int
process_dc_fsm_update(lua_State *L, const char *domain_id, const char *key,
  const char *value)
{
    int err = 0;

    int type_id = lua_getglobal(L, "uo");
    assert(type_id == LUA_TTABLE);

    type_id = lua_getfield(L, -1, "update_dc_fsm");
    if (type_id != LUA_TFUNCTION) {
        LOG_ERROR("type id was %d, to be %d", type_id, LUA_TFUNCTION);
        return EINVAL;
    }
    lua_getglobal(L, "uo");
    lua_pushstring(L, domain_id);
    lua_pushstring(L, key);
    if (value) {
        LOG_DEBUG("[L=%p] on '%s' apply (k, v) = ('%s', '%s')", L, domain_id,
          key, value);
        lua_pushstring(L, value);
    } else {
        LOG_DEBUG("[L=%p] on '%s' apply (k, v) = ('%s', nil)", L, domain_id,
          key);
        lua_pushnil(L);
    }
    int lerr = lua_pcall(L, 4, 0, 0);
    if (lerr != LUA_OK)
        err = -1; // tentative

    return err;
}

// mainloop of UO main
static int
loop(struct ctx *ctx)
{
    int err;

    char session_id[sizeof(ctx->session_id)] = {};
    snprintf(session_id, sizeof(session_id) - 1, "Proc_%u", getpid());
    err = init_ctx(ctx, session_id);
    if (err)
        return err;

    err = start_uo_script(ctx);
    LOG_DEBUG("start_uo_script() -> %d", err);

    {
        lua_State *L = grab_lua_state(ctx);
        if (L) {
            int type_id = lua_getglobal(L, "uo");
            assert(type_id == LUA_TTABLE);
            type_id = lua_getfield(L, -1, "cleanup");
            lua_getglobal(L, "uo");
            LOG_DEBUG(" call claenup L=%p", L);
            int lerr = lua_pcall(L, 1, 0, 0);
            if (lerr != LUA_OK) {
                LOG_DEBUG(" claenup failed %d", lerr);
                // give-up recovery
            }
            release_lua_state(ctx, L);
        }
    }

    fini_ctx(ctx);

    return err;
}

static void
usage(const char *prog)
{
    fprintf(stderr,
      "Usage: %s -p PREFIX -s SCRIPT_URL"
      "\n",
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
        LOG_DEBUG("base_dir='%s'", base_dir);
    }
    while ((opt = getopt(argc, argv, "hp:s:t:")) != -1) {
        switch (opt) {
        case 'p': // prefix (== campaign id)
            ctx->prefix = optarg;
            LOG_DEBUG("prefix : '%s'", ctx->prefix);
            break;
        case 's': // script path
            ctx->script_url = optarg;
            LOG_DEBUG("script_url : '%s'", ctx->script_url);
            break;
        default:
            LOG_DEBUG("invalid option '%c'", opt);
            usage(prog);
            return EINVAL;
        case 'h':
            usage(prog);
            exit(EXIT_SUCCESS);
        }
    }
    return 0;
}

int
main(int argc, char **argv)
{
    int err;
    struct ctx ctx = {};

    ctx.debug = 1;

    err = parse_args(&ctx, argc, argv);

    if (!err)
        err = loop(&ctx);

    if (err)
        exit(EXIT_FAILURE);

    return 0;
}
