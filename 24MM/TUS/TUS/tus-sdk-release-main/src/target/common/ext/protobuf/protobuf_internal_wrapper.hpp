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
#ifndef TMC_PROTOBUF_INTERNAL_WRAPPER_H__
#define TMC_PROTOBUF_INTERNAL_WRAPPER_H__

#include <lua.hpp>

int tmc_protobuf_internal_read_tag(lua_State *L) noexcept;
int tmc_protobuf_internal_inline_greedy_string_parser(lua_State *L) noexcept;
int tmc_protobuf_internal_verify_utf8(lua_State *L) noexcept;
int tmc_protobuf_internal_read_varint64(lua_State *L) noexcept;
int tmc_protobuf_internal_read_varint32(lua_State *L) noexcept;
int tmc_protobuf_internal_read_varint_zigzag64(lua_State *L) noexcept;
int tmc_protobuf_internal_read_varint_zigzag32(lua_State *L) noexcept;
int tmc_protobuf_internal_expect_tag(lua_State *L) noexcept;
int tmc_protobuf_internal_unknown_field_parse(lua_State *L) noexcept;
int tmc_protobuf_internal_unaligned_load(lua_State *L) noexcept;
int tmc_protobuf_internal_packed_int32_parser(lua_State *L) noexcept;
int tmc_protobuf_internal_packed_uint32_parser(lua_State *L) noexcept;
int tmc_protobuf_internal_packed_int64_parser(lua_State *L) noexcept;
int tmc_protobuf_internal_packed_uint64_parser(lua_State *L) noexcept;
int tmc_protobuf_internal_packed_sint32_parser(lua_State *L) noexcept;
int tmc_protobuf_internal_packed_sint64_parser(lua_State *L) noexcept;
int tmc_protobuf_internal_packed_enum_parser(lua_State *L) noexcept;
int tmc_protobuf_internal_packed_bool_parser(lua_State *L) noexcept;
int tmc_protobuf_internal_packed_fixed32_parser(lua_State *L) noexcept;
int tmc_protobuf_internal_packed_sfixed32_parser(lua_State *L) noexcept;
int tmc_protobuf_internal_packed_fixed64_parser(lua_State *L) noexcept;
int tmc_protobuf_internal_packed_sfixed64_parser(lua_State *L) noexcept;
int tmc_protobuf_internal_packed_float_parser(lua_State *L) noexcept;
int tmc_protobuf_internal_packed_double_parser(lua_State *L) noexcept;

#endif // TMC_PROTOBUF_INTERNAL_WRAPPER_H__
