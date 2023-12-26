/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */

#pragma once

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

int http_init(void);

int http_fetch_to_buffer(const char *url, void *buffer, size_t limit);

struct lua_State;
int http_push_fecher_to_lua(struct lua_State *L);

#ifdef __cplusplus
}
#endif // __cplusplus
