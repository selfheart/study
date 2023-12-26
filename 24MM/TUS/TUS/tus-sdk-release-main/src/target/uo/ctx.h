/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */

#pragma once

#include <limits.h>
#include <sys/types.h>

struct ctx {
    struct worker {
        // valid on the patent process
        pid_t pid;
        enum state {
            WORKER_STATE_NONE = 0,
            WORKER_STATE_RUNNING,
            WORKER_STATE_SHUTDOWN,
        } state;
        void (*stop)(struct ctx *ctx, struct worker *wk);
        int exit_code;
        int exit_signal;

        // valid on both
        int sock; // (each end of a socket)
        int debug;

        // valid on child
        pid_t ppid; // parent pid, for debugging
    } uo_main, httpd;

    int debug;
};

// may use protobuf if complex messages are needed
enum ipc_code {
    IPC_CODE__RESERVED = 0,
    IPC_CODE_TERMINATE = 1, // shutting down
    IPC_CODE_PING = 0x10,   //

    IPC_CODE_HTTPD_SET_TUP_URL = 0x100,
};

int worker_recv(struct worker *wk, enum ipc_code *code, int *flags, void *buf,
  size_t limit, size_t *actual);

int worker_notify(struct worker *wk, enum ipc_code code, int flags,
  const void *buf, ssize_t size);
#define IPC_BUF_SIZE (128 + PATH_MAX) // tentative

#define MAX_SESSION_ID_BYTES 16
