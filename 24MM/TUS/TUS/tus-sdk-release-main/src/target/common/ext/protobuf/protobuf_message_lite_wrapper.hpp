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
#ifndef TMC_PROTOBUF_MESSAGE_LITE_WRAPPER_H__
#define TMC_PROTOBUF_MESSAGE_LITE_WRAPPER_H__

#include <lua.hpp>

int tmc_protobuf_message_lite_new(lua_State *L) noexcept;
int tmc_protobuf_message_lite_delete(lua_State *L) noexcept;

int tmc_protobuf_message_lite_get_cached_size(lua_State *L) noexcept;

int tmc_protobuf_message_lite_set_cached_size(lua_State *L) noexcept;
int tmc_protobuf_message_lite_get_internal_metadata(lua_State *L) noexcept;

int tmc_protobuf_message_lite_serialize_to_string(lua_State *L) noexcept;
int tmc_protobuf_message_lite_parse_from_string(lua_State *L) noexcept;

#endif // TMC_PROTOBUF_MESSAGE_LITE_WRAPPER_H__
