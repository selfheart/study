/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */

#include <errno.h>
#include <grpcpp/grpcpp.h>

#include "logger.h"
#include "runner_dc_grpc.h"

// generated from 'dc.proto'
#include "dc.grpc.pb.h"

struct grpc_domaincontroller_stub *
grpc_domaincontroller_new(const char *target)
{

    std::unique_ptr<TUS::DomainController::Stub> stub =
      TUS::DomainController::NewStub(
        grpc::CreateChannel(target, grpc::InsecureChannelCredentials()));
    return (struct grpc_domaincontroller_stub *)stub.release();
}

void
grpc_domaincontroller_delete(struct grpc_domaincontroller_stub *stub)
{
    TUS::DomainController::Stub *s = (TUS::DomainController::Stub *)stub;
    delete s;
}

// arg is opaque 'struct ctx'
int
grpc_domaincontroller_poll_update(struct ctx *ctx,
  struct grpc_domaincontroller_stub *stub, const char *session_id,
  const char *domain_id, int *connected,
  int (*handle_fsm_env)(struct ctx *ctx, const char *k, const char *v,
    const char *domain_id),
  int (*handle_async_op)(struct ctx *ctx, uint32_t tag, int result_code,
    void *result, size_t size))
{
    grpc::ClientContext context;

    TUS::Session session;
    session.set_id(session_id);

    TUS::Updates updates; // events from DC
    TUS::DomainController::Stub *s = (TUS::DomainController::Stub *)stub;
    grpc::Status status = s->PollEnv(&context, session, &updates);

    if (status.ok()) {
        if ((*connected) == 0) {
            char connected_val[32] = "true"; // expected to be a string for now
            struct timespec tv = {};
            if (clock_gettime(CLOCK_MONOTONIC, &tv) == 0) {
                snprintf(connected_val, sizeof(connected_val) - 1, "%lu.%04lu",
                  tv.tv_sec, tv.tv_nsec / 1000 / 1000);
            }
            *connected = 1;
            LOG_DEBUG("mark the connection as alive(%s)", connected_val);
            (void)handle_fsm_env(ctx, "connected", connected_val, domain_id);
        }
        int err = updates.result_code();
        if (err)
            return err;
        if (handle_fsm_env) {
            for (int i = 0; i < updates.pairs_size(); i++) {
                TUS::PairedStr pair = updates.pairs(i);
                err = handle_fsm_env(ctx, pair.key().c_str(),
                  pair.value().c_str(), domain_id);
                if (err)
                    break; // ToDo
            }
        }
        if (handle_async_op) {
            for (int i = 0; i < updates.arets_size(); i++) {
                TUS::AsyncResult aret = updates.arets(i);
                const char *result = aret.result_text().c_str();
                err = handle_async_op(ctx, aret.tag(), aret.result_code(),
                  (void *)result, 0);
                if (err)
                    break; // ToDo
            }
        }
        return err;
    } else {
        if (*connected) {
            LOG_DEBUG("mark the connection(cur:%d) as dead", *connected);
            *connected = 0;
            (void)handle_fsm_env(ctx, "connected", NULL, domain_id);
        }
        return -1;
    }
}
