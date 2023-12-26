/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */

#pragma once

struct ctx;
struct worker;

struct httpd_config {
    const char *path_base;
    int port;
    // ToDo: parameters for SSL?
};
// to be called from a child process
// 'config' shall be a pointer to 'struct httpd_config'
int httpd_main(struct worker *wk, void *config);

// ToDo: locate a usable listening port dynamically?
#define HTTPD_DEFAULT_PORT 7681
