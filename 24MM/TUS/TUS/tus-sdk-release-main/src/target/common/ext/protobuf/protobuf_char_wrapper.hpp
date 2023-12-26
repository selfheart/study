/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */
/* TMC CONFIDENTIAL
* $JITDFLibId$
* Copyright (C) 2022 TOYOTA MOTOR CORPORATION
* All Rights Reserved.
*/
#ifndef TMC_PROTOBUF_CHAR_WRAPPER_H__
#define TMC_PROTOBUF_CHAR_WRAPPER_H__

#include <lua.hpp>

int tmc_protobuf_char_eq(lua_State *L) noexcept;
int tmc_protobuf_char_add(lua_State *L) noexcept;
int tmc_protobuf_char_sub(lua_State *L) noexcept;

#endif // TMC_PROTOBUF_CHAR_WRAPPER_H__
