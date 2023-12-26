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
#include <poll.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "ctx.h"
#include "logger.h"

#ifdef HAS_LIBWEBSOCKETS
#include "worker/httpd.h"
#endif

static void
dump_lua_error(lua_State *L, const char *desc)
{
    const char *errmsg = lua_tostring(L, -1);
    LOG_ERROR("Lua error[%s]:\n----\n"
              "%s"
              "\n----\n",
      desc ? desc : "", errmsg ? errmsg : "(?)");
}

static void
dump_lua_stack(lua_State *L, int loglevel, const char *desc)
{
    int depth = lua_gettop(L);

    LOG__VALIST(loglevel, "(Lua stack dump[%s]: depth = %d)", desc ? desc : "",
      depth);
    while (depth > 0) {
#define CUR L, depth
        switch (lua_type(CUR)) {
        case LUA_TNIL: // 0
            LOG__VALIST(loglevel, "- %d: LUA_TNIL", depth);
            break;
        case LUA_TBOOLEAN: // 1
            LOG__VALIST(loglevel, "- %d: LUA_TBOOLEAN (%s)", depth,
              lua_toboolean(CUR) ? "true" : "false");
            break;
        case LUA_TLIGHTUSERDATA: // 2
            LOG__VALIST(loglevel, "- %d: LUA_TLIGHTUSERDATA (p=%p)", depth,
              lua_touserdata(CUR));
            break;
        case LUA_TNUMBER: // 3
            LOG__VALIST(loglevel, "- %d: LUA_TNUMBER (%f)", depth,
              lua_tonumberx(CUR, NULL));
            break;
        case LUA_TSTRING: // 4
        {
            size_t len;
            const char *p = lua_tolstring(CUR, &len);
            LOG__VALIST(loglevel, "- %d: LUA_TSTRING ('%s', len=%zu)", depth, p,
              len);
            break;
        }
        case LUA_TTABLE: // 5
            LOG__VALIST(loglevel, "- %d: LUA_TTABLE rawlen=%lld", depth,
              (long long)lua_rawlen(CUR));
            break;
        case LUA_TFUNCTION: // 6
            LOG__VALIST(loglevel, "- %d: LUA_TFUNCTION (func=%p)", depth,
              lua_tocfunction(CUR));
            break;
        case LUA_TUSERDATA: // 7
            LOG__VALIST(loglevel, "- %d: LUA_TUSERDATA (block_addr=%p)", depth,
              lua_touserdata(CUR));
            break;
        case LUA_TTHREAD: // 8
            LOG__VALIST(loglevel, "- %d: LUA_TTHREAD (lua_State=%p)", depth,
              lua_tothread(CUR));
            break;
        default:
            LOG__VALIST(loglevel, "- %d: (unknown type %d)", depth,
              lua_type(CUR));
            break;
        }
#undef CUR
        depth--;
    }
}

// for LUA_REGISTRYINDEX
static char lua_registry_key[] = "uo_main";
static struct worker *
get_wk_from_luastate(lua_State *L)
{
    struct worker *wk;

    // dump_lua_stack(L, LOG_LV_DEBUG, "PRE");

    lua_pushlightuserdata(L, lua_registry_key);
    lua_gettable(L, LUA_REGISTRYINDEX);
    wk = lua_touserdata(L, -1);
    lua_pop(L, 1); // pop the 'wk' (LUA_TLIGHTUSERDATA)

    // dump_lua_stack(L, LOG_LV_DEBUG, "POST");

    return wk;
}

static int
tuslua__has_feature(lua_State *L)
{
    int num_results = 0;
    int support_level = 0;
    // no optional named feature is supported (for now)
    const char *name = lua_tostring(L, -1);
    if (name) {
        LOG_DEBUG("%s(%s') -> %d", __func__, name, support_level);
    } else {
        LOG_DEBUG("%s(NULL) -> %d", __func__, support_level);
    }

    lua_pushinteger(L, support_level);
    num_results++;

    return num_results;
}

static int uomain__handle_ipc(struct worker *wk,
  int (*callback)(enum ipc_code code, int arg, void *buf, size_t size,
    void *callback_arg),
  void *callback_arg, int milliseconds);
// for uomain__handle_ipc()
static int
callback__wait_for_tup(enum ipc_code code, int arg, void *buf, size_t size,
  void *callback_arg)
{
    int err = 0;

    (void)buf;
    (void)size;
    int *dest = callback_arg;

    switch (code) {
    case IPC_CODE_HTTPD_SET_TUP_URL:
        if (arg == -1) {
            *dest = -1; // failure
        } else {
            *dest = 1;
        }
        break;
    default:
        err = EINVAL;
        break;
    }
    return err;
}

// to be called from Lua as config("key", "value"),
// returns nil / error code
static int
tuslua__tus_config(lua_State *L)
{
    int err = 0;

    int num_results = 0;
    struct worker *wk = get_wk_from_luastate(L);
    if (!wk) {
        // Lua registry has not yet set?
        LOG_ERROR("miss-configured? (wk=%p)", wk);
        err = EINVAL;
    } else {
        // dump_lua_stack(L, LOG_LV_DEBUG, __func__);
        size_t klen;
        const char *key = luaL_tolstring(L, 1, &klen);
        size_t vlen;
        const char *value = luaL_tolstring(L, 2, &vlen);
        LOG_WARN("%s: '%s'", key, value);
        if (!key) {
            // dump_lua_stack(L, LOG_LV_WARN, __func__);
            err = EINVAL;
        } else if (strncmp(key, "HTTPD_SET_TUP_URL", klen + 1) == 0) {
            if (value) {
                int err = worker_notify(wk, IPC_CODE_HTTPD_SET_TUP_URL, 0,
                  value, vlen);
                if (!err) {
                    int got_reply = 0;
                    const int wait_milliseconds = 2000;
                    while (got_reply == 0) {
                        LOG_WARN("wait for HTTPD_SET_TUP_URL: '%s'", value);
                        err = uomain__handle_ipc(wk, callback__wait_for_tup,
                          &got_reply, wait_milliseconds);
                        if (err)
                            break;
                    }
                    if (!err && got_reply < 0) {
                        err = EIO; // httpd reported an failure
                    }
                    LOG_WARN("(%d, err=%d)", got_reply, err);
                }
            } else {
                LOG_ERROR(" (%p, %p)", wk, value);
            }
        } else {
            // unknown key
            const char *value = lua_tostring(L, -1);
            if (value) {
                LOG_ERROR("invalid key '%s', value='%s'", key, value);
                err = ENOENT;
            } else {
                LOG_ERROR("invalid key '%s', value type=%d", key,
                  lua_type(L, -1));
                err = EINVAL;
            }
        }
    }
    if (err) {
        LOG_ERROR("err=%d", err);
        lua_pushinteger(L, err);
        num_results++;
    } else {
        LOG_HIDDEN("num_results=%d", num_results);
    }
    return num_results;
}

// to be called from Lua as config("key", "value"),
// returns nil / error code
static int
tuslua__syslog(lua_State *L)
{
    int loglevel;
    // dump_lua_stack(L, LOG_LV_DEBUG, __func__);
    switch (lua_type(L, -2)) {
    case LUA_TNUMBER: {
        const lua_Integer val = lua_tointeger(L, -2);
        if (val != (int)val) {
            LOG_ERROR("invalid  loglevel %lld", (long long)val);
            return 0;
        }
        loglevel = val;
        break;
    }
    case LUA_TNIL:
        loglevel = LOG_LV_INFO;
        break;
    default:
        LOG_ERROR("invalid loglevel type %d", lua_type(L, -2));
        return 0;
    }

    const char *msg = lua_tostring(L, -1);
    log_valist(loglevel, "(Lua) %s\t", msg ? msg : "");
    return 0;
}

static int
uomain__handle_ipc(struct worker *wk,
  int (*callback)(enum ipc_code code, int arg, void *buf, size_t size,
    void *callback_arg),
  void *callback_arg, int milliseconds)
{
    if (!wk)
        return EINVAL;
    if (wk->ppid > 0) {
        pid_t ppid = getppid();
        if (wk->ppid != ppid) {
            LOG_WARN("reparented %d -> %d\n", wk->ppid, ppid);
            return EINTR; // parent process seems to have gone
        }
    }
    int err = 0;
    struct pollfd fds[] = { {
      .fd = wk->sock,
      .events = POLLIN | POLLHUP | POLLERR,
    } };
    const int nfd = poll(fds, sizeof(fds) / sizeof(fds[0]), milliseconds);

    // return early if interrupted
    if (nfd > 0) {
        if (fds[0].revents && POLLIN) {
            LOG_DEBUG("ready to read from sock %d", wk->sock);
            enum ipc_code code = IPC_CODE__RESERVED;
            int arg = 0;
            size_t actual = 0;
            char buf[IPC_BUF_SIZE] = {};

            err = worker_recv(wk, &code, &arg, buf, sizeof(buf), &actual);
            if (!err) {
                switch (code) {
                case IPC_CODE_PING:
                    LOG_DEBUG("got ping (code=%#x) from %#x", code, arg);
                    err = worker_notify(wk, code, getpid(), NULL, 0);
                    break;

                case IPC_CODE_TERMINATE:
                    LOG_INFO("got IPC_CODE_TERMINATE arg=%d", arg);
                    err = -1; // fixme: better to raise a Lua error?
                    break;

                default:
                    if (callback) {
                        err = callback(code, arg, buf, actual, callback_arg);
                    } else {
                        LOG_ERROR("unexpected code %#x?", code);
                        err = EINVAL;
                    }
                    break;
                }
            }
        } else {
            LOG_INFO("disconnected? %#x", fds[0].revents);
            err = -1; // fixme: better to raise a Lua error?
        }
    } else if (nfd < 0) {
        err = errno;
        LOG_INFO("interrupted? %#x", err);
    }
    return err;
}

// sleep at most N secs, (may return earlier).
// to be called from Lua as sleep(N)
// returns nil / error code
static int
tuslua__sleep(lua_State *L)
{
    int err = 0;
    int num_results = 0;
    int is_int;
    lua_Integer seconds = lua_tointegerx(L, -1, &is_int);

    if (!is_int || seconds < 0 || 3600 < seconds) {
        // sleeping nearly forever is considered be a caller's bug
        LOG_ERROR("shall not sleep for %lld (is_int=%d) secs",
          (long long)seconds, is_int);
        err = EINVAL;
    } else {
        err = uomain__handle_ipc(get_wk_from_luastate(L), NULL, NULL,
          seconds * 1000);
    }

    if (err) {
        lua_pushinteger(L, err);
        num_results++;
    }
    return num_results;
}

// TUS UO main
lua_State *
prepare_luavm(struct worker *wk)
{
    lua_State *L = luaL_newstate();
    if (L) {
        // enable standard libraries. fixme: apply some restrictions?
        luaL_openlibs(L);

        // pre-init for 'uo'

        /* let registry[lua_registryindex] = wk, for get_wk_from_luastate() */
        lua_pushlightuserdata(L, lua_registry_key);
        lua_pushlightuserdata(L, wk);
        lua_settable(L, LUA_REGISTRYINDEX);
        LOG_DEBUG("pushed wk=%p to Lua registry", wk);

        // add VM extensions to be used from UO main (uo.lua & main.lua)
        lua_newtable(L);
        {
            // let _tuslua.NAME = tuslua__NAME()
#define EXPORT_TO_LUA(NAME)                   \
    do {                                      \
        lua_pushstring(L, #NAME);             \
        lua_pushcfunction(L, tuslua__##NAME); \
        lua_settable(L, -3);                  \
    } while (0)
            EXPORT_TO_LUA(syslog);
            EXPORT_TO_LUA(has_feature);
            EXPORT_TO_LUA(tus_config);
            EXPORT_TO_LUA(sleep);
#undef EXPORT_TO_LUA
        }
        lua_setglobal(L, "_tuslua");

        // prepared global '_tuslua', load module 'uo'
#define MODULE_UO "uo" // expect to have "uo.lua" in ./
        int lua_err = luaL_dostring(L, "uo = require('" MODULE_UO "')");
        if (lua_err != LUA_OK) {
            dump_lua_error(L, "failed to load '" MODULE_UO "'");
            lua_close(L);
            return NULL; // FIXME
        }
        LOG_DEBUG("loaded '" MODULE_UO "' for %p", L);
#undef MODULE_UO
        // hide '_tuslua'
        lua_pushnil(L);
        lua_setglobal(L, "_tuslua");

        // dump_lua_stack(L, LOG_LV_DEBUG, __func__);
    }
    return L;
}

int
uo_main(struct worker *wk, void *config)
{
    (void)config;
    assert(wk->pid == 0); // to be called from a child process

    int err;

    logger_set_prefix("M:%#x\t", getpid());
    LOG_DEBUG("started child process %#x", getpid());

    {
        // note: wk->sock is expected to be in the blocking mode
        enum ipc_code code = IPC_CODE__RESERVED;
        int arg = 0;
        err = worker_recv(wk, &code, &arg, NULL, 0, NULL);
        if (!err) {
            if (code == IPC_CODE_PING) {
                LOG_WARN("got ping (code=%#x) from %#x", code, arg);
                wk->ppid = arg;
                err = worker_notify(wk, code, getpid(), NULL, 0);
            } else {
                LOG_ERROR("sync error? got %#x, to be PING", code);
                err = EINVAL;
            }
        }
    }

    lua_State *L;
    if (!(L = prepare_luavm(wk))) {
        // failed to init Lua VM for UO-main
        return ENOMEM;
    }

    if (!err) {
        // ToDo: tweak loaded 'uo' before starting main.lua?

        // UO main's main loop
        int lua_err = luaL_loadfile(L, "./main.lua");
        if (lua_err == LUA_OK) {
            lua_err = lua_pcall(L, 0, 1, 0);
            switch (lua_err) {
            case LUA_OK:
                if (lua_type(L, -1) != LUA_TNIL) {
                    LOG_ERROR("stop UO main; error '%s'", lua_tostring(L, -1));
                    err = EINVAL; // FIXME:
                } else {
                    LOG_INFO("UO main completed (%d)", lua_type(L, -1));
                }
                break;
            default:
                err = EINVAL;
                LOG_DEBUG("Lua error %d -> %d", lua_err, err);
                dump_lua_error(L, "running main.lua");
                break;
            }
        } else {
            dump_lua_error(L, "loading main.lua");
        }
    }

    // lua_close(L);
    return err;
}

static void
usage(const char *prog)
{
    // FIXME:
    fprintf(stderr,
      "Usage: %s"
      "\n",
      prog);
}

static int
init_ctx(struct ctx *ctx, int argc, char **argv)
{
    int opt;
    (void)ctx;
    // set current directory to the executable's parent
    // to let Lua VM find extensions in it
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

    while ((opt = getopt(argc, argv, "h")) != -1) {
        switch (opt) {
        case 'h':
            usage(prog);
            exit(EXIT_SUCCESS);
        }
    }

    // ToDo: put global init here
    return 0;
}

static void
worker_stop(struct worker *wk)
{
    if (wk->state != WORKER_STATE_RUNNING) {
        LOG_DEBUG("%p is not running (%d)", wk, wk->state);
        return;
    }
    wk->state = WORKER_STATE_SHUTDOWN;
    if (wk->pid > 0) {
        // has a child process
        siginfo_t si = {};

        int err = worker_notify(wk, IPC_CODE_TERMINATE, 0, __func__, -1);
        if (err) {
            if (err != ECONNREFUSED) {
                LOG_ERROR("IPC_CODE_TERMINATE failed %d", err);
                kill(wk->pid, SIGTERM);
                // ToDo: retry w/SIGKILL on error?
            } else {
                LOG_DEBUG("%p seems to have gone", wk);
            }
        }

        LOG_DEBUG("waiting pid %#x", wk->pid);
        if (waitid(P_PID, wk->pid, &si, WEXITED)) {
            perror("waitid");
            LOG_WARN("lost pid %#x (==%d)?", wk->pid, wk->pid);
        } else if (wk->pid == si.si_pid) {
            switch (si.si_code) {
            case CLD_EXITED:
                LOG_DEBUG("waitid(%#x) -> %#x has exited (%d)", wk->pid,
                  si.si_pid, si.si_status);
                wk->exit_code = si.si_status;
                wk->exit_signal = -1; // not killed
                break;
            case CLD_KILLED:
            case CLD_DUMPED:
                LOG_DEBUG("waitid(%#x) -> %#x signal %d (si_code=%d)", wk->pid,
                  si.si_pid, si.si_status, si.si_code);
                wk->exit_code = -1;
                wk->exit_signal = si.si_status;
                break;
            default:
                LOG_DEBUG("waitid(%#x) -> unexpected si_code %#x?", wk->pid,
                  si.si_code);
                break;
            }
        } else {
            LOG_ERROR("unexpected si_pid %#x, which shall be %#x", si.si_pid,
              wk->pid);
        }
    } else {
        // waiting or 0 or -1 will cause unexpected results
        LOG_DEBUG("cannot wait pid %#x", wk->pid);
    }
    //
    wk->state = WORKER_STATE_NONE;
    if (wk->sock != -1) {
        close(wk->sock);
        wk->sock = -1;
    }
}

static void
fini_ctx(struct ctx *ctx)
{
    LOG_INFO("(finalizing ctx %p)", ctx);

    LOG_INFO("(- stopping HTTPd %#x)", ctx->httpd.pid);
    worker_stop(&ctx->httpd);

    LOG_INFO("(- stopping UO main %#x)", ctx->uo_main.pid);
    worker_stop(&ctx->uo_main);
}

static int
worker_start(int (*loop)(struct worker *wk, void *config), struct worker *wk,
  void *config)
{
    int err;
    int sv[2];
    if (wk->state != WORKER_STATE_NONE) {
        LOG_ERROR("already running? %d", wk->state);
        return EINVAL;
    }
    wk->state = WORKER_STATE_RUNNING;
    wk->sock = -1;
    wk->exit_code = -1;   // not yet exited
    wk->exit_signal = -1; // no signal has been delivered

    // anon socket pair as an IPC channel
    const int protocol = 0;
    if (socketpair(AF_UNIX, SOCK_DGRAM, protocol, sv) == 0) {
        // parent will keep sv[0], sv[1] is for a child
        LOG_HIDDEN("sockpair() -> [%d, %d]", sv[0], sv[1]);

        wk->pid = fork();
        if (wk->pid == -1) {
            err = errno;
            perror("fork");
            close(sv[0]);
            close(sv[1]);
        } else {
            if (wk->pid == 0) {
                // in a child process, shall not return from here
                setsid(); // detach controling-terminal
                close(sv[0]);
                wk->sock = sv[1];
                err = (loop)(wk, config);
                if (err == 0) {
                    LOG_INFO("worker %#x finished", getpid());
                } else {
                    LOG_WARN("worker %#x finished %d", getpid(), err);
                }
                exit(err); // parent shall catch the err by waitid()
            }
            // (in the parent process)
            close(sv[1]);
            wk->sock = sv[0];
            LOG_DEBUG("spawned child pid=%#x(%d)", wk->pid, wk->pid);
            {
                // queue ping to wait startup of the worker
                err = worker_notify(wk, IPC_CODE_PING, getpid(), "ping", -1);
                if (!err) {
                    enum ipc_code ret_code;
                    int arg = 0;
                    LOG_HIDDEN("(waiting reply from %d)", wk->pid);
                    err = worker_recv(wk, &ret_code, &arg, NULL, 0, NULL);
                    if (!err) {
                        if (ret_code == IPC_CODE_PING) {
                            LOG_INFO("got a reply, %#x", arg);
                        } else {
                            LOG_WARN("unexpected reply, %#x", ret_code);
                            // seems to be alive
                        }
                    }
                } else {
                    LOG_ERROR("failed to ping (%d)", err);
                }
            }
        }

    } else {
        err = errno;
        perror("socketpair");
    }

    if (err) {
        wk->state = WORKER_STATE_NONE;
        LOG_ERROR("failed to start %p (%d)", wk, err);
    }
    return err;
}

static int
monitor_workers(struct ctx *ctx)
{
    int milliseconds = 1000;
    int nfd = 0;
    int ret = 0;

    struct worker *workers[] = {
        &(ctx->httpd),
        &(ctx->uo_main),
    };
    struct pollfd fds[sizeof(workers) / sizeof(workers[0])];

    for (size_t i = 0; i < sizeof(fds) / sizeof(fds[0]); i++) {
        const struct worker *wk = workers[i];
        if (wk->pid > 0) {
            siginfo_t si = {};
            if (waitid(P_PID, wk->pid, &si, WEXITED | WNOHANG | WNOWAIT) == 0) {
                if (wk->pid == si.si_pid) {
                    // ToDo: may better to re-start for some cases?
                    // (like "HTTPd has died unexpectedly")
                    const char *reason = NULL;
                    switch (si.si_code) {
                    case CLD_EXITED:
                        reason = "CLD_EXITED";
                        break;
                    case CLD_KILLED:
                        reason = "CLD_KILLED";
                        break;
                    case CLD_DUMPED:
                        reason = "CLD_DUMPED";
                        break;
                    }
                    if (reason) {
                        LOG_WARN("pid %#x has gone %s(%d) -> %d", wk->pid,
                          reason, si.si_code, si.si_status);
                        return -1;
                    }
                }
            }
        }
        fds[i].fd = wk->sock;
        fds[i].events = POLLIN | POLLHUP | POLLERR;
    }

    nfd = poll(fds, sizeof(fds) / sizeof(fds[0]), milliseconds);
    if (nfd > 0) {
        LOG_HIDDEN("poll() -> nfd=%d", nfd);
        for (size_t i = 0; i < sizeof(fds) / sizeof(fds[0]); i++) {
            struct worker *wk = workers[i];
            // has events
            if (fds[i].revents & POLLHUP) {
                LOG_WARN("POLLHUP for %d", fds[i].fd);
                // todo: destruct?
                continue;
            }
            if (fds[i].revents & POLLERR) {
                LOG_ERROR("POLLERR for %d", fds[i].fd);
                // todo: destruct?
                continue;
            }
            if (fds[i].revents & POLLIN) {
                enum ipc_code code = IPC_CODE__RESERVED;
                int arg = 0;
                char buf[IPC_BUF_SIZE] = {};
                size_t actual = 0;
                int err = worker_recv(wk, &code, &arg, buf, sizeof(buf) - 2,
                  &actual);
                if (err) {
                    LOG_ERROR("err=%d", err);
                    // not fatal, connection may be lost at any time
                } else {
                    switch (code) {
                    case IPC_CODE_HTTPD_SET_TUP_URL: {
                        struct worker *dest = NULL;
                        if (wk == &(ctx->uo_main)) {
                            // forward a request
                            dest = &(ctx->httpd);
                        } else if (wk == &(ctx->httpd)) {
                            // route a reply
                            dest = &(ctx->uo_main);
                        }
                        if (!dest) {
                            err = EINVAL;
                        } else {
                            err = worker_notify(dest, code, arg, buf, actual);
                        }
                        if (err) {
                            LOG_ERROR("failed to route HTTPD_SET_TUP_URL %d",
                              err);
                        }
                        break;
                    }
                    default:
                        LOG_WARN("unhandled code %#x (arg=%d, '%s')", code, arg,
                          buf);
                        break;
                    }
                }
            }
        }
    }
    if (nfd == -1) {
        ret = errno;
        if (ret != EINTR) {
            perror("poll");
        } else {
            LOG_WARN("(interrupted, errno=%d)", ret);
        }
    }
    return ret;
}

static volatile sig_atomic_t interrupted = 0;
static void
sigaction_cleanup(int signum, siginfo_t *info, void *ucontext)
{
    (void)info;
    (void)ucontext;
    LOG_HIDDEN("signum=%d", signum);
    interrupted = 1;
}

int
main(int argc, char **argv)
{
    int err;
    struct ctx ctx = {};

    // tentative
    ctx.debug = 1;

    // prefer EPIPE
    signal(SIGPIPE, SIG_IGN);

    err = init_ctx(&ctx, argc, argv);
    if (!err) {
        {
#ifdef HAS_LIBWEBSOCKETS
            // FIXME: to be read from 'uo/config'
            struct httpd_config httpd_config = {
                // may be tested by
                //  curl -v http://localhost:7681/tus/tup/ipkg1/data
                .path_base = "/tus",
                .port = HTTPD_DEFAULT_PORT,
            };
            // HTTP server process
            err = worker_start(httpd_main, &ctx.httpd, &httpd_config);
#endif
        }
        if (!err) {
            err = worker_start(uo_main, &ctx.uo_main, NULL);
        }

        if (!err) {
            logger_set_prefix("*:%#x\t", getpid());
            {
                // let poll() return EINTR in SIGINT
                // to start clean shutdown
                struct sigaction sa = {
                    .sa_sigaction = sigaction_cleanup,
                    .sa_flags = SA_SIGINFO,
                };
                sigemptyset(&sa.sa_mask);
                sigaction(SIGINT, &sa, NULL);
            }
            while (!interrupted) {
                LOG_HIDDEN("monitoring %p", &ctx);
                err = monitor_workers(&ctx);
                if (err) {
                    break;
                }
            }
        }
        fini_ctx(&ctx);
    }

    LOG_WARN("exiting, err=%d", err);
    if (err)
        exit(EXIT_FAILURE);

    return EXIT_SUCCESS;
}
