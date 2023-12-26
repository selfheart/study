/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */

#pragma once

#include <pthread.h>
#include <queue.h>

struct update_event {
    SIMPLEQ_ENTRY(update_event) next;
    // ToDo: prepare dedicated pages for entries
    char key[32]; // sizes are tentative
    char value[64];
};

#define SESSION_ID_SIZE (16) // FIXME: tentaive

// ToDo: better to store as a fixed-size table?
struct ctx;
struct async_op_state {
    TAILQ_ENTRY(async_op_state) next;
    char session_id[SESSION_ID_SIZE];
    uint32_t tag;
    int result_code;
    int from_uo;
    void *result;
    size_t size;

    //  run() may set ->reslt referencing ->arg
    void (*run)(struct ctx *ctx, struct async_op_state *as);
    void *arg; // URL etc.
    // release() will collect ->result and ->arg.
    void (*release)(struct ctx *ctx, struct async_op_state *as);
};

struct tus_dc_platform;
struct ctx {
    pthread_mutex_t mtx;
    pthread_cond_t cv;

    int shutdown;
    int (*lock_ctx)(struct ctx *ctx);
    void (*unlock_ctx)(struct ctx *ctx);
    int (*wait_ctx)(struct ctx *ctx);
    int (*wakeup_ctx)(struct ctx *ctx);
    int (*push_async_result)(struct ctx *ctx, uint32_t async_tag, int code,
      const char *value, size_t result_size);

    // PAL
    struct tus_dc_platform *platform;

    // DC main
    lua_State *L;

    // DC sctipt
    struct {
        lua_State *L;
        int refs;
        int pending_release;
        double next_wakeup; // either NaN or a coerced monotonic timestamp
    } script;
    char session_id[SESSION_ID_SIZE]; // ToDo: keep an uuid for current session?
    char
      config_base_dir[PATH_MAX]; // config file directory. ToDo: keep as dirfd?
    char domain_id[NAME_MAX];    // domain identifier (used as UO.domains[n].id)

    // DC FSM state notifications
    SIMPLEQ_HEAD(, update_event) queued_updates;

    //
    struct {
        pthread_t th;
        int enabled; // true if 'th' is to be collected
        uint32_t last_tag;

        // async exec ops to be procesed by an async ececutor
        TAILQ_HEAD(, async_op_state) pending_exec;

        // completed asyncc ops to be reported to UO
        TAILQ_HEAD(, async_op_state) done_uo;
        // completed asyncc ops to be consumed locally
        TAILQ_HEAD(, async_op_state) done_local;
    } async;

    struct {
        pthread_t th;
        int running;
        int error;
        char address[64]; // "host:port"
    } grpc;               // gPRC server

    int debug;
};

struct tus_dc_updater_call_context {
    lua_State *L;
    size_t returns; // num of args pushed to the Lua stack (as return values)
};

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

struct update_event *pop_update_event(struct ctx *ctx);
struct async_op_state *pop_async_done_uo(struct ctx *ctx);
int wait_update_event(struct ctx *ctx, const char *session_id);

int queue_async_op(struct ctx *ctx, void *arg, const char *session_id,
  void (*release)(struct ctx *ctx, struct async_op_state *as),
  void (*run)(struct ctx *ctx, struct async_op_state *as), uint32_t tag);

// make a Lua state to execute TUP DC scripte (not DC main)
lua_State *get_script_state(struct ctx *ctx, const char *session_id);
void release_script_state(lua_State *L);

// init "struct tus_dc_platform" by setting its 'util'
int prepare_dc_platform_utility(struct tus_dc_platform *platform,
  struct ctx *ctx);

#ifdef __cplusplus
}
#endif // __cplusplus
