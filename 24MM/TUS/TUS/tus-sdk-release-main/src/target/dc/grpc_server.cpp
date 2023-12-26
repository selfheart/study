/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */

#include <grpcpp/grpcpp.h>
#include <unistd.h>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include "common.h"
#include "dc.grpc.pb.h"
#include "fetch.h"
#include "grpc_server.h"
#include "logger.h"
#include "platform/platform.h"

// fixme: to be fefined
#define UNDEFINED_ERROR (-1)

// used to classify struct async_op_state
static void
release_async_op_state_ctx(struct ctx *ctx, struct async_op_state *as)
{
    if (as->release == release_async_op_state_ctx) {
        auto p = static_cast<char *>(as->arg);
        free(p);
    } else {
        LOG_ERROR("got release_context=%p, which shall be %p", as->release,
          release_async_op_state_ctx);
    }
}

// used to classify struct async_op_state
static void
run_url_from_uo(struct ctx *ctx, struct async_op_state *ao)
{
    const char *url = (char *)(ao->arg);
    LOG_DEBUG(" AO tag=%d", ao->tag);
    LOG_DEBUG(" AO url='%s'", url);

    // FIXME: tentative, shall read incrementally (by lua_load() + reader)
    size_t limit = (16U << 10); // 16 KiB
    void *buffer = malloc(limit);
    int err;
    if (buffer) {
        memset(buffer, 0, limit);
        err = http_fetch_to_buffer(url, buffer, limit);
        if (!err) {
            // use SC script context
            lua_State *L = get_script_state(ctx, ao->session_id);
            LOG_DEBUG("exec script[%s] on %p:\n\t----\n\t%s\n\t----\n",
              ao->session_id, L, (char *)buffer);
            if (L) {
                int lerr = luaL_dostring(L, (char *)buffer);
                if (lerr == LUA_OK) {
                    LOG_DEBUG("executed (lerr=%d)", lerr);
                    err = 0;
                } else {
                    // shall map lerr to a TUS error code
                    const char *errmsg = lua_tostring(L, -1);
                    LOG_ERROR("(lerr=%d)\n----\n"
                              "%s"
                              "\n----\n",
                      lerr, errmsg ? errmsg : "(?)");
                    err = -1;
                }
                release_script_state(L);
            } else {
                err = EINVAL; // use better error code?
            }
        } else {
            LOG_ERROR("fetch_to_buffer(%s) -> %d", url, err);
        }
        free(buffer);
    } else {
        err = ENOMEM;
    }
    // caller will re-qeue this ao
    ao->result_code = err;
}

class DomainControllerImpl final : public TUS::DomainController::Service {
private:
    struct ctx *ctx;
    lua_State *grab_lua_state(const char *session_id)
    {
        return get_script_state(this->ctx, session_id);
    }

    void release_lua_state(lua_State *L)
    {
        lua_settop(L, 0);
        release_script_state(L);
    }

public:
    DomainControllerImpl(struct ctx *ctx)
        : ctx(ctx)
    {
    }

    grpc::Status GetVersions(grpc::ServerContext *context,
      const TUS::Names *names, TUS::Versions *versions) override
    {
        (void)context;
        int err = 0;
        const size_t num = names->name().size();
        LOG_DEBUG("num=%zu", num);
        if (num <= 0)
            return grpc::Status::OK;
        if (num > INT_MAX) // must be a bug
            return grpc::Status::CANCELLED;

        // execute in DC-main context
        lua_State *L = this->grab_lua_state(NULL);
        if (L) {
            int lua_type = lua_getglobal(L, "dc");
            assert(lua_type == LUA_TTABLE);
            int top = lua_gettop(L);
            for (size_t i = 0; i < num; i++) {
                lua_settop(L, top);
                lua_type = lua_getfield(L, -1, "get_version");
                if (lua_type == LUA_TFUNCTION) {
                    const int nargs = 4;
                    const int nresults = 2;
                    const int msgh = 0;

                    const std::string key = names->name(i);
                    LOG_DEBUG("req '%s'", key.c_str());
                    lua_getglobal(L, "dc");         // self
                    lua_pushstring(L, key.c_str()); // target_spec

                    // todo: support passing variation/context from UO with Names
                    lua_pushinteger(L,
                      DC_VERSION_VARIATION_CURRENT); // default variation_code
                    lua_pushinteger(L, DC_VERSION_CONTEXT_ANY); // default context_code
                    int lerr = lua_pcall(L, nargs, nresults, msgh);
                    if (lerr == LUA_OK) {
                        const char *v = lua_tostring(L, -2);
                        if (!v) {
                            int is_int = 0;
                            lua_Integer code = lua_tointegerx(L, -1, &is_int);
                            if (!is_int || code == 0 || code != (int32_t)code) {
                                LOG_ERROR("coerce error code(%lld, %d)",
                                  (long long)code, is_int);
                                code = -1;
                            } else {
                                LOG_ERROR("failed to get version, code = %d",
                                  (int)code);
                            }
                            err = code;
                            break;
                        }
                        LOG_DEBUG(" -> value = '%s'", v);
                        TUS::PairedStr *pair = versions->add_pairs();
                        pair->set_key(key);
                        if (v) {
                            pair->set_value(v);
                        } else {
                            pair->set_value("");
                        }
                    } else {
                        LOG_ERROR("lua_pcall(), %d\n", lerr);
                        const char *errmsg = lua_tostring(L, -1);
                        if (errmsg) {
                            LOG_ERROR(
                              "%p failed to exec (%d):\n----\n%s\n----\n", L,
                              lerr, errmsg);
                        } else {
                            LOG_ERROR("%p failed to exec (%d)\n", L, lerr);
                        }
                        err = UNDEFINED_ERROR;
                    }
                } else {
                    LOG_ERROR("missing 'get_version' as a function? %d",
                      lua_type);
                    err = UNDEFINED_ERROR;
                }
                if (err)
                    break;
            }
            this->release_lua_state(L);
        }
        if (err) {
            versions->set_result_code(err);
            return grpc::Status::CANCELLED;
        }
        return grpc::Status::OK;
    }

    grpc::Status Execute(grpc::ServerContext *context,
      const TUS::Script *script, TUS::Response *response) override
    {
        grpc::Status grpc_ret;

        (void)context;
        LOG_DEBUG("got script[session=%s]:\n"
                  "\t----\n"
                  "\t%s\n"
                  "\t----",
          script->session_id().c_str(), script->body().c_str());

        lua_State *L = this->grab_lua_state(script->session_id().c_str());
        if (L) {
            LOG_DEBUG("Lua state = %p", L);
            int pre = lua_gettop(L);
            int lerr = luaL_loadstring(L, script->body().c_str());
            if (lerr == LUA_OK) {
                lerr = lua_pcall(L, 0, LUA_MULTRET, 0);
                if (lerr == LUA_OK && pre < lua_gettop(L)) {
                    // LOG_ERROR("%d, gettop() %d -> %d", lerr, pre,
                    // lua_gettop(L));
                    const char *result = lua_tostring(L, -1);
                    response->set_result_text(result);
                    lua_pop(L, lua_gettop(L) - pre);
                }
            }
            response->set_result_code(lerr);
            if (lerr != LUA_OK) {
                const char *errmsg = lua_tostring(L, -1);
                if (errmsg) {
                    LOG_ERROR("%p failed to exec (%d):\n----\n%s\n----\n", L,
                      lerr, errmsg);
                } else {
                    LOG_ERROR("failed to exec (%d)\n", lerr);
                }
                grpc_ret = grpc::Status::CANCELLED;
            } else
                grpc_ret = grpc::Status::OK;
            this->release_lua_state(L);
        }
        return grpc_ret;
    }

    grpc::Status ExecuteAsync(grpc::ServerContext *context,
      const TUS::ExecuteAsyncParams *params, TUS::Response *response) override
    {
        (void)context;

        int ret = 0;

        char *arg = nullptr;

        arg = strdup(params->url().c_str());
        if (arg) {
            int err = queue_async_op(this->ctx, arg,
              params->session_id().c_str(), release_async_op_state_ctx,
              run_url_from_uo, params->tag());
            if (err) {
                ret = UNDEFINED_ERROR;
                free(arg);
            } else {
                // 'arg' is now owned by the queue
            }
        } else {
            ret = UNDEFINED_ERROR;
        }
        response->set_result_code(ret);
        // response->set_result_text();
        return grpc::Status::OK;
    }

    grpc::Status PollEnv(grpc::ServerContext *context, const TUS::Session *uo,
      TUS::Updates *updates) override
    {
        int err = 0;

        (void)context;
        const char *session_id = uo->id().c_str();
        LOG_DEBUG("polled for session id: %s", session_id);
        int n = 0;
        while (!err) {
            struct update_event *ue;
            while ((ue = pop_update_event(this->ctx)) != NULL) {
                TUS::PairedStr *pair = updates->add_pairs();
                pair->set_key(ue->key);
                pair->set_value(ue->value);
                n++;
            }
            struct async_op_state *ao;
            while ((ao = pop_async_done_uo(this->ctx)) != NULL) {
                TUS::AsyncResult *aret = updates->add_arets();
                aret->set_tag(ao->tag);
                aret->set_result_code(ao->result_code);
                if (ao->result)
                    aret->set_result_text((char *)(ao->result));
                if (ao->release)
                    (ao->release)(this->ctx, ao);
                n++;
            }
            if (n == 0) {
                LOG_DEBUG("wait for events (%d)", n);
                int ret = wait_update_event(this->ctx, session_id);
                if (ret != 0) {
                    LOG_DEBUG("interrupted (%d)", ret);
                    break;
                }
            } else {
                LOG_DEBUG("got %d events", n);
                break;
            }
        }
        // ToDo: may wait & retry for efficeiency

        updates->set_result_code(err);

        return err ? grpc::Status::CANCELLED : grpc::Status::OK;
    }

    grpc::Status SetEnvironment(grpc::ServerContext *context,
      const TUS::SetEnvironmentParams *params, TUS::Response *response) override
    {
        int err = 0;

        (void)context;
        LOG_DEBUG("setenv[session '%s']: %s = %s",
          params->session().id().c_str(), params->key().c_str(),
          params->value().c_str());
        (void)context;

        // execute in DC-script context
        lua_State *L = this->grab_lua_state(params->session().id().c_str());
        if (L) {
            int lua_type = lua_getglobal(L, "dc");
            assert(lua_type == LUA_TTABLE);
            lua_type = lua_getfield(L, -1, "fsm");
            assert(lua_type == LUA_TTABLE);
            lua_type = lua_getfield(L, -1, "set_environment");
            assert(lua_type == LUA_TFUNCTION);

            lua_type = lua_getfield(L, -3, "fsm");
            assert(lua_type == LUA_TTABLE);

            lua_pushstring(L, params->key().c_str());
            lua_pushstring(L, params->value().c_str());

            const int nargs = 3;
            const int nresults = 1;
            const int msgh = 0;

            int lerr = lua_pcall(L, nargs, nresults, msgh);
            if (lerr != LUA_OK) {
                err = UNDEFINED_ERROR;
                response->set_result_code(err);
            }
            this->release_lua_state(L);
        }
        return err ? grpc::Status::CANCELLED : grpc::Status::OK;
    }
};

void *
grpc_server_worker(void *arg)
{
    int err = 0;
    struct ctx *ctx = static_cast<struct ctx *>(arg);
    if (!ctx) {
        LOG_ERROR("expected struct ctx *: '%p'", ctx);
        return NULL;
    }

    const char *server_address = ctx->grpc.address;

    DomainControllerImpl service(ctx);

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());

    LOG_DEBUG("server_address: '%s' (%d)", server_address, server != nullptr);
    if (server == nullptr) {
        LOG_ERROR("failed to bind target '%s'", server_address);
        err = EINVAL;
    }

    ctx->lock_ctx(ctx);
    if (err) {
        ctx->grpc.running = 0;
        ctx->grpc.error = err;
    } else {
        ctx->grpc.running = 1;
        ctx->grpc.error = 0;
        ctx->wakeup_ctx(ctx);
        ctx->unlock_ctx(ctx);

        LOG_DEBUG("started: '%s'", server_address);
        server->Wait();
        LOG_DEBUG("finished: '%s'", server_address);

        ctx->lock_ctx(ctx);
        // ToDo: update 'error'?
        ctx->grpc.running = 0;
    }
    ctx->wakeup_ctx(ctx);
    ctx->unlock_ctx(ctx);

    return NULL;
}
