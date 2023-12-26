/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */
/* TMC CONFIDENTIAL
* $JITDFLibId$
* Copyright (C) 2021 TOYOTA MOTOR CORPORATION
* All Rights Reserved.
*/
#ifndef TMC_PROTOBUF_PARSE_CONTEXT_WRAPPER_H__
#define TMC_PROTOBUF_PARSE_CONTEXT_WRAPPER_H__

#include <lua.hpp>

int tmc_protobuf_parse_context_done(lua_State *L);
int tmc_protobuf_parse_context_parse_message(lua_State *L);
int tmc_protobuf_parse_context_data_available(lua_State *L);

#endif // TMC_PROTOBUF_PARSE_CONTEXT_WRAPPER_H__
