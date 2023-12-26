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
#ifndef TMC_PROTOBUF_EPS_COPY_OUTPUT_STREAM_WRAPPER_H__
#define TMC_PROTOBUF_EPS_COPY_OUTPUT_STREAM_WRAPPER_H__

#include <lua.hpp>

int tmc_protobuf_eps_copy_output_stream_ensure_space(lua_State *L) noexcept;
int tmc_protobuf_eps_copy_output_stream_write_string_maybe_aliased(lua_State *L) noexcept;
int tmc_protobuf_eps_copy_output_stream_write_bytes_maybe_aliased(lua_State *L) noexcept;
int tmc_protobuf_eps_copy_output_stream_write_string(lua_State *L) noexcept;
int tmc_protobuf_eps_copy_output_stream_write_bytes(lua_State *L) noexcept;
int tmc_protobuf_eps_copy_output_stream_write_raw(lua_State *L) noexcept;
int tmc_protobuf_eps_copy_output_stream_write_int32_packed(lua_State *L) noexcept;
int tmc_protobuf_eps_copy_output_stream_write_uint32_packed(lua_State *L) noexcept;
int tmc_protobuf_eps_copy_output_stream_write_sint32_packed(lua_State *L) noexcept;
int tmc_protobuf_eps_copy_output_stream_write_int64_packed(lua_State *L) noexcept;
int tmc_protobuf_eps_copy_output_stream_write_uint64_packed(lua_State *L) noexcept;
int tmc_protobuf_eps_copy_output_stream_write_sint64_packed(lua_State *L) noexcept;
int tmc_protobuf_eps_copy_output_stream_write_enum_packed(lua_State *L) noexcept;
int tmc_protobuf_eps_copy_output_stream_write_fixed_packed(lua_State *L) noexcept;

#endif // TMC_PROTOBUF_EPS_COPY_OUTPUT_STREAM_WRAPPER_H__
