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
#ifndef TMC_PROTOBUF_WIRE_FORMAT_LITE_WRAPPER_H__
#define TMC_PROTOBUF_WIRE_FORMAT_LITE_WRAPPER_H__

#include <lua.hpp>

int tmc_protobuf_wire_format_lite_verify_utf8_string(lua_State *L) noexcept;
int tmc_protobuf_wire_format_lite_internal_write_message(lua_State *L) noexcept;
int tmc_protobuf_wire_format_lite_int32_size(lua_State *L) noexcept;
int tmc_protobuf_wire_format_lite_int64_size(lua_State *L) noexcept;
int tmc_protobuf_wire_format_lite_uint32_size(lua_State *L) noexcept;
int tmc_protobuf_wire_format_lite_uint64_size(lua_State *L) noexcept;
int tmc_protobuf_wire_format_lite_sint32_size(lua_State *L) noexcept;
int tmc_protobuf_wire_format_lite_sint64_size(lua_State *L) noexcept;
int tmc_protobuf_wire_format_lite_enum_size(lua_State *L) noexcept;
int tmc_protobuf_wire_format_lite_string_size(lua_State *L) noexcept;
int tmc_protobuf_wire_format_lite_bytes_size(lua_State *L) noexcept;
int tmc_protobuf_wire_format_lite_message_size(lua_State *L) noexcept;
int tmc_protobuf_wire_format_lite_write_int32_to_array(lua_State *L) noexcept;
int tmc_protobuf_wire_format_lite_write_int64_to_array(lua_State *L) noexcept;
int tmc_protobuf_wire_format_lite_write_uint32_to_array(lua_State *L) noexcept;
int tmc_protobuf_wire_format_lite_write_uint64_to_array(lua_State *L) noexcept;
int tmc_protobuf_wire_format_lite_write_sint32_to_array(lua_State *L) noexcept;
int tmc_protobuf_wire_format_lite_write_sint64_to_array(lua_State *L) noexcept;
int tmc_protobuf_wire_format_lite_write_fixed32_to_array(lua_State *L) noexcept;
int tmc_protobuf_wire_format_lite_write_fixed64_to_array(lua_State *L) noexcept;
int tmc_protobuf_wire_format_lite_write_sfixed32_to_array(lua_State *L) noexcept;
int tmc_protobuf_wire_format_lite_write_sfixed64_to_array(lua_State *L) noexcept;
int tmc_protobuf_wire_format_lite_write_float_to_array(lua_State *L) noexcept;
int tmc_protobuf_wire_format_lite_write_double_to_array(lua_State *L) noexcept;
int tmc_protobuf_wire_format_lite_write_bool_to_array(lua_State *L) noexcept;
int tmc_protobuf_wire_format_lite_write_enum_to_array(lua_State *L) noexcept;

#endif // TMC_PROTOBUF_WIRE_FORMAT_LITE_WRAPPER_H__
