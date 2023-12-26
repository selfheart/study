/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */

#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifdef MISSING_C11_THREADS
// degrade to have per-process (not per thred) prefix
#define thread_local
#else
// shall be able to use thread-local-storage
#include <threads.h>
#endif

#include "logger.h"

static const char reset[] = "\x1b[0m";
static const char fg_red[] = "\x1b[31m";
// static const char fg_green[] = "\x1b[32m";
static const char fg_yellow[] = "\x1b[33m";
// static const char fg_blue[] = "\x1b[34m";
static const char fg_magenta[] = "\x1b[35m";
static const char fg_cyan[] = "\x1b[36m";
static volatile int loglevel = LOG_LV_DEBUG;

static inline void
push(char **head, size_t *consumed, const void *buf)
{
    size_t len = strlen(buf);
    memcpy(*head, buf, len);
    *consumed += len;
    *head += len;
}

static thread_local char prefix[32] = {};

void
logger_set_prefix(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(prefix, sizeof(prefix), fmt, ap);
    va_end(ap);
    prefix[sizeof(prefix) - 1] = '\0';
}

void
log_valist_raw(int lv, const char *fmt, va_list ap)
{
    if (loglevel >= lv) {
        char line[PATH_MAX + 32] = {};
        char *head = line;
        size_t consumed = 0;
        int enable_color = isatty(STDERR_FILENO);
        int reserve = 2; // reserve 2 bytes (for LF & NUL)
        if (prefix[0]) {
            push(&head, &consumed, prefix);
        }
        if (enable_color) {
            // to put 'reset' to the tail of log
            reserve += sizeof(reset) - 1;

            switch (lv) {
            case LOG_LV_CRITICAL:
                push(&head, &consumed, fg_magenta);
                break;
            case LOG_LV_ERROR:
                push(&head, &consumed, fg_red);
                break;
            case LOG_LV_WARN:
                push(&head, &consumed, fg_yellow);
                break;
            case LOG_LV_DEBUG:
                push(&head, &consumed, fg_cyan);
                break;
            default:
                break;
            }
        }
        vsnprintf(head, sizeof(line) - consumed - reserve, fmt, ap);
        ssize_t len = strnlen(line, sizeof(line) - reserve);

        if (enable_color) {
            sprintf(line + len, "%s", reset);
            len += sizeof(reset) - 1;
            consumed += sizeof(reset) - 1;
        }

        line[len] = '\n';
        len++;
        off_t off = 0;
        while (len > off) {
            ssize_t written = write(STDERR_FILENO, line + off, len - off);
            if (written <= 0)
                break;
            off += written;
        }
    }
}

void
log_valist(int lv, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    log_valist_raw(lv, fmt, ap);
    va_end(ap);
}
