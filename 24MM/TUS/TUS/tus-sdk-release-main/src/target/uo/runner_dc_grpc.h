/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */

#pragma once

struct grpc_domaincontroller_stub;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

struct grpc_domaincontroller_stub *grpc_domaincontroller_new(
  const char *target);
void grpc_domaincontroller_delete(struct grpc_domaincontroller_stub *stub);

struct ctx;
int grpc_domaincontroller_poll_update(struct ctx *ctx,
  struct grpc_domaincontroller_stub *stub, const char *session_id,
  const char *domain_id, int *known_connected,
  int (*handle_fsm_env)(struct ctx *ctx, const char *k, const char *v,
    const char *domain_id),
  int (*handle_async_op)(struct ctx *ctx, uint32_t tag, int result_code,
    void *result, size_t size));

void *gen_cli(const char *id,
  int (*handle_pair)(const char *k, const char *v, void *arg), void *arg);

#ifdef __cplusplus
}
#endif // __cplusplus
