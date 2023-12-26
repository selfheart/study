/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "ctx.h"
#include "logger.h"

// tentative IPC message format, to be hidden from outside
struct ipc_header {
    int code; // enum ipc_code
    int arg;
};

int
worker_recv(struct worker *wk, enum ipc_code *code, int *arg, void *buf,
  size_t limit, size_t *actual)
{
    struct ipc_header header = {};
    char temp[IPC_BUF_SIZE] = {};
    int err = 0;

    if (actual)
        *actual = 0;
    if (code)
        *code = IPC_CODE__RESERVED;
    if (arg)
        *arg = 0;

    // todo: omit memcpy if limit == sizeof(temp)?
    assert(limit <= sizeof(temp));

    struct iovec iov[] = {
        {
          .iov_base = &header,
          .iov_len = sizeof(header),
        },
        {
          .iov_base = temp,
          .iov_len = sizeof(temp),
        },
    };
    struct msghdr mh = {
        .msg_iov = iov,
        .msg_iovlen = sizeof(iov) / sizeof(iov[0]),
    };

    ssize_t bytes = recvmsg(wk->sock, &mh, 0);
    if (bytes >= (ssize_t)sizeof(header)) {
        bytes -= sizeof(header);
        if (actual)
            *actual = bytes;
        if (buf) {
            if ((size_t)bytes < limit) {
                LOG_HIDDEN("got actual=%zd", bytes);
                memcpy(buf, temp, bytes);
            } else {
                LOG_WARN("clipped msg, %zd -> %zd", bytes, limit);
                memcpy(buf, temp, limit);
            }
        }
        if (code)
            *code = header.code;
        if (arg)
            *arg = header.arg;
    } else {
        if (!errno) {
            LOG_DEBUG("incomplete msg? (%zd)", bytes);
            err = EINVAL;
        } else if (errno == EAGAIN) {
            // LOG_DEBUG("no msg (%d)", errno);
        } else {
            // disconnected?
            err = errno;
            perror("recvmsg");
            LOG_ERROR("socket fd %d, bytes=%ld, err=%d", wk->sock, bytes, err);
        }
    }
    return err;
}

int
worker_notify(struct worker *wk, enum ipc_code code, int arg, const void *buf,
  ssize_t size)
{
    int err = 0;
    struct ipc_header header = {
        .code = code,
        .arg = arg,
    };
    const ssize_t limit = PATH_MAX + 1;
    if (size == -1) {
        size = strnlen(buf, limit);
        if (size >= limit) {
            LOG_ERROR("%zd > %zd", size, limit);
            return EINVAL;
        }
        LOG_HIDDEN("size=%zd from '%s'", size, (char *)buf);
    }
    struct iovec iov[] = {
        {
          .iov_base = &header,
          .iov_len = sizeof(header),
        },
        {
          .iov_base = (void *)buf,
          .iov_len = size,
        },
    };
    const struct msghdr mh = {
        .msg_iov = iov,
        .msg_iovlen = sizeof(iov) / sizeof(iov[0]),
    };
    ssize_t bytes = sendmsg(wk->sock, &mh, 0);
    if (bytes < 0) {
        err = errno;
        LOG_WARN("sendmsg(%d) -> err %d", wk->sock, err);
    } else {
        LOG_HIDDEN("sendmsg(code=%#x, data size=%zd) -> %zd", code, size,
          bytes);
    }
    return err;
}
