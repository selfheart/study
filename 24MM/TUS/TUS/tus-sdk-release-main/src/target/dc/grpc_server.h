/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */

#pragma once

struct grpc_server_config {
    const char *target;
    void *server;
    void *ctx;
};

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// to be used with pthread_create()
// 'ctx' shall be 'struct ctx *'
void *grpc_server_worker(void *ctx);

#ifdef __cplusplus
}
#endif // __cplusplus
