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
#include <pthread.h>
#include <queue.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "logger.h"
#include "platform/platform.h"

static void
log_error(const struct tus_dc_platform_utility *util, const char *fmt, ...)
{
    if (util) {
        va_list ap;
        va_start(ap, fmt);
        log_valist_raw(LOG_LV_ERROR, fmt, ap);
        va_end(ap);
    }
}

static void
log_info(const struct tus_dc_platform_utility *util, const char *fmt, ...)
{
    if (util) {
        va_list ap;
        va_start(ap, fmt);
        log_valist_raw(LOG_LV_INFO, fmt, ap);
        va_end(ap);
    }
}

static void
log_debug(const struct tus_dc_platform_utility *util, const char *fmt, ...)
{
    if (util) {
        va_list ap;
        va_start(ap, fmt);
        log_valist_raw(LOG_LV_DEBUG, fmt, ap);
        va_end(ap);
    }
}

static int
generate_async_tag(const struct tus_dc_platform_utility *util)
{
    int tag = TUS_DC_PAL_INVALID_ASYNC_TAG;
    struct ctx *ctx = util->ctx;

    if (ctx) {
        // note: util->ctx shall have already been locked
        ctx->async.last_tag++;
        if (ctx->async.last_tag == TUS_DC_PAL_INVALID_ASYNC_TAG)
            ctx->async.last_tag++;
        tag = ctx->async.last_tag;
    }
    return tag;
}

// see set_async_results_from_lua() in main.c, to be unified
static int
set_async_results(const struct tus_dc_platform_utility *util, int tag, int code)
{
    int err;
    struct ctx *ctx = util->ctx;

    if (ctx) {
        // note: util->ctx shall have already been locked
        const void *aux = NULL;
        const size_t aux_size = 0;
        err = ctx->push_async_result(ctx, tag, code, aux, aux_size);
        ctx->unlock_ctx(ctx);
    } else {
        err = EINVAL;
    }
    return err; // fixme: not yer implementated
}

// see dc.lua, self.call_updater_from_lua() is called with "native, key, ..."
// return only user-supplied args (== '...')
static const int userarg_base = 2;

static int
updater__compare_updater_type(const struct tus_dc_updater_type_id *lhs,
  const struct tus_dc_updater_type_id *rhs, void *arg)
{
    (void)arg; // 'arg' is placed to make this function qsort()-compatible

    // fixme: tentative, depends on 'struct tus_dc_updater_type_id'
    return memcmp(lhs, rhs, sizeof(*lhs));
}

static int
updater__count_arguments(struct tus_dc_updater_call_context *call_ctx)
{

    if (call_ctx && call_ctx->L) {
        lua_State *L = call_ctx->L;
        int num_args = lua_gettop(L) - userarg_base;
        LOG_DEBUG("num_args=%d", num_args);
        return num_args;
    } else {
        LOG_ERROR("no argument can be used for the call_ctx: %p", call_ctx);
        return 0;
    }
}

static int
updater__get_arg_as_int64(struct tus_dc_updater_call_context *call_ctx,
  int arg_index, int64_t *val)
{
    if (call_ctx && call_ctx->L && arg_index >= 0) {
        lua_State *L = call_ctx->L;
        // +1 to convert from C-style to Lua-style index
        int effctive = userarg_base + arg_index + 1;
        switch (lua_type(L, effctive)) {
        case LUA_TNUMBER:
            *val = (int64_t)lua_tointeger(L, effctive);
            return 0; // no error
        default:
            LOG_ERROR("bad type for effctive=%d: %d (!=TNUMBER)", effctive,
              lua_type(L, effctive));
            return -1; // fixme
        }
    } else {
        LOG_ERROR("%p %p %d", call_ctx, call_ctx ? call_ctx->L : NULL,
          arg_index);
        return -1; // fixme
    }
}

static int
updater__copy_arg_data(struct tus_dc_updater_call_context *call_ctx,
  int arg_index, void *buffer, size_t limit)
{
    if (call_ctx && call_ctx->L && arg_index >= 0) {
        lua_State *L = call_ctx->L;
        // +1 to convert from C-style to Lua-style index
        int effctive = userarg_base + arg_index + 1;
        switch (lua_type(L, effctive)) {
        case LUA_TSTRING: {
            size_t len = 0;
            const char *s = lua_tolstring(L, effctive, &len);
            if (s && len <= limit) {
                memset(buffer, 0, limit);
                memcpy(buffer, s, len);
                return 0; // no error
            } else {
                return -1; // fixme
            }
        }
        default:
            LOG_ERROR("bad type for effctive=%d: %d (!= TSTRING)", effctive,
              lua_type(L, effctive));
            return -1; // fixme
        }
    } else {
        LOG_ERROR("%p %p %d", call_ctx, call_ctx ? call_ctx->L : NULL,
          arg_index);
        return -1; // fixme
    }
}

static int
updater__add_result_int64(struct tus_dc_updater_call_context *call_ctx,
  int64_t result)
{
    if (call_ctx && call_ctx->L) {
        lua_State *L = call_ctx->L;
        lua_pushinteger(L, result);
        LOG_INFO("pushed #%zd (%lld)", call_ctx->returns, (long long)result);
        call_ctx->returns++;
    } else {
        LOG_ERROR("%p %p", call_ctx, call_ctx ? call_ctx->L : NULL);
        return -1; // fixme
    }
    return 0;
}

// features exposed for a platform-specific implementations
static struct tus_dc_platform_utility pal_util = {
    .log_error = log_error,
    .log_info = log_info,
    .log_debug = log_debug,

    .async = {
	.generate_tag = generate_async_tag,
	.set_results = set_async_results,
    },

    .updater = {
	.compare_updater_type = updater__compare_updater_type,
	.count_arguments = updater__count_arguments,
	.get_arg_as_int64 = updater__get_arg_as_int64,
	.copy_arg_data = updater__copy_arg_data,
	.add_result_int64 = updater__add_result_int64,
    }
};

int
prepare_dc_platform_utility(struct tus_dc_platform *platform, struct ctx *ctx)
{
    if (!platform || !ctx) {
        return EINVAL;
    }
    memset(platform, 0, sizeof(*platform));

    // note: cannot do 'platform->util->ctx = ctx' here, since 'util' is const
    pal_util.ctx = ctx; // keep ctx for internal use
    platform->util = &pal_util;
    return 0;
}
