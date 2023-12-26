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
#include <lua.hpp>

#include "protobuf_helper.hpp"
#include "protobuf_meta_context.hpp"
#include "protobuf_char_wrapper.hpp"
#include "protobuf_message_lite_wrapper.hpp"
#include "protobuf_internal_wrapper.hpp"
#include "protobuf_wire_format_lite_wrapper.hpp"
#include "protobuf_internal_metadata_wrapper.hpp"
#include "protobuf_parse_context_wrapper.hpp"
#include "protobuf_eps_copy_output_stream_wrapper.hpp"

#define tmc_protobuf_lua_setclass(L, class_name, meta_context, methods, index_methods, meta_methods, constants) \
    do                                                                                                          \
    {                                                                                                           \
        luaL_newmetatable(L, meta_context);                                                                     \
        luaL_newlib(L, index_methods);                                                                          \
        lua_setfield(L, -2, "__index");                                                                         \
        luaL_setfuncs(L, meta_methods, 0);                                                                      \
        lua_pop(L, 1);                                                                                          \
        luaL_newlib(L, methods);                                                                                \
        tmc_protobuf_lua_setnullptr(L, meta_context);                                                           \
        tmc_protobuf_lua_setintegers(L, constants);                                                             \
        lua_setfield(L, -2, class_name);                                                                        \
    } while (0)

struct constant
{
    const char *name;
    const int value;
};

static const struct luaL_Reg nullmethod[] = {
    {NULL, NULL}};
static const constant nullconstant[] = {
    {NULL, 0}};

static const struct luaL_Reg char_meta_methods[] = {
    {"__eq", tmc_protobuf_char_eq},
    {"__add", tmc_protobuf_char_add},
    {"__sub", tmc_protobuf_char_sub},
    {NULL, NULL}};

static const struct luaL_Reg message_lite_methods[] = {
    {"new", tmc_protobuf_message_lite_new},
    {"delete", tmc_protobuf_message_lite_delete},
    {"get_cached_size", tmc_protobuf_message_lite_get_cached_size},
    {"set_cached_size", tmc_protobuf_message_lite_set_cached_size},
    {"get_internal_metadata", tmc_protobuf_message_lite_get_internal_metadata},
    {"serialize_to_string", tmc_protobuf_message_lite_serialize_to_string},
    {"parse_from_string", tmc_protobuf_message_lite_parse_from_string},
    {NULL, NULL}};
static const struct luaL_Reg message_lite_index_methods[] = {
    {"delete", tmc_protobuf_message_lite_delete},
    {"get_cached_size", tmc_protobuf_message_lite_get_cached_size},
    {"set_cached_size", tmc_protobuf_message_lite_set_cached_size},
    {"get_internal_metadata", tmc_protobuf_message_lite_get_internal_metadata},
    {"serialize_to_string", tmc_protobuf_message_lite_serialize_to_string},
    {"parse_from_string", tmc_protobuf_message_lite_parse_from_string},
    {NULL, NULL}};

static const struct luaL_Reg internal_methods[] = {
    {"read_tag", tmc_protobuf_internal_read_tag},
    {"inline_greedy_string_parser", tmc_protobuf_internal_inline_greedy_string_parser},
    {"verify_utf8", tmc_protobuf_internal_verify_utf8},
    {"read_varint64", tmc_protobuf_internal_read_varint64},
    {"read_varint32", tmc_protobuf_internal_read_varint32},
    {"read_varint_zigzag64", tmc_protobuf_internal_read_varint_zigzag64},
    {"read_varint_zigzag32", tmc_protobuf_internal_read_varint_zigzag32},
    {"expect_tag", tmc_protobuf_internal_expect_tag},
    {"unknown_field_parse", tmc_protobuf_internal_unknown_field_parse},
    {"unaligned_load", tmc_protobuf_internal_unaligned_load},
    {"packed_int32_parser", tmc_protobuf_internal_packed_int32_parser},
    {"packed_uint32_parser", tmc_protobuf_internal_packed_uint32_parser},
    {"packed_int64_parser", tmc_protobuf_internal_packed_int64_parser},
    {"packed_uint64_parser", tmc_protobuf_internal_packed_uint64_parser},
    {"packed_sint32_parser", tmc_protobuf_internal_packed_sint32_parser},
    {"packed_sint64_parser", tmc_protobuf_internal_packed_sint64_parser},
    {"packed_enum_parser", tmc_protobuf_internal_packed_enum_parser},
    {"packed_bool_parser", tmc_protobuf_internal_packed_bool_parser},
    {"packed_fixed32_parser", tmc_protobuf_internal_packed_fixed32_parser},
    {"packed_sfixed32_parser", tmc_protobuf_internal_packed_sfixed32_parser},
    {"packed_fixed64_parser", tmc_protobuf_internal_packed_fixed64_parser},
    {"packed_sfixed64_parser", tmc_protobuf_internal_packed_sfixed64_parser},
    {"packed_float_parser", tmc_protobuf_internal_packed_float_parser},
    {"packed_double_parser", tmc_protobuf_internal_packed_double_parser},
    {NULL, NULL}};
static const struct constant internal_constants[] = {
    {"INT32", CPPTYPE_INT32},
    {"INT64", CPPTYPE_INT64},
    {"UINT32", CPPTYPE_UINT32},
    {"UINT64", CPPTYPE_UINT64},
    {"DOUBLE", CPPTYPE_DOUBLE},
    {"FLOAT", CPPTYPE_FLOAT},
    {"BOOL", CPPTYPE_BOOL},
    {"ENUM", CPPTYPE_ENUM},
    {NULL, 0}};

static const struct luaL_Reg wire_format_lite_methods[] = {
    {"verify_utf8_string", tmc_protobuf_wire_format_lite_verify_utf8_string},
    {"internal_write_message", tmc_protobuf_wire_format_lite_internal_write_message},
    {"int32_size", tmc_protobuf_wire_format_lite_int32_size},
    {"int64_size", tmc_protobuf_wire_format_lite_int64_size},
    {"uint32_size", tmc_protobuf_wire_format_lite_uint32_size},
    {"uint64_size", tmc_protobuf_wire_format_lite_uint64_size},
    {"sint32_size", tmc_protobuf_wire_format_lite_sint32_size},
    {"sint64_size", tmc_protobuf_wire_format_lite_sint64_size},
    {"enum_size", tmc_protobuf_wire_format_lite_enum_size},
    {"string_size", tmc_protobuf_wire_format_lite_string_size},
    {"bytes_size", tmc_protobuf_wire_format_lite_bytes_size},
    {"message_size", tmc_protobuf_wire_format_lite_message_size},
    {"write_int32_to_array", tmc_protobuf_wire_format_lite_write_int32_to_array},
    {"write_int64_to_array", tmc_protobuf_wire_format_lite_write_int64_to_array},
    {"write_uint32_to_array", tmc_protobuf_wire_format_lite_write_uint32_to_array},
    {"write_uint64_to_array", tmc_protobuf_wire_format_lite_write_uint64_to_array},
    {"write_sint32_to_array", tmc_protobuf_wire_format_lite_write_sint32_to_array},
    {"write_sint64_to_array", tmc_protobuf_wire_format_lite_write_sint64_to_array},
    {"write_fixed32_to_array", tmc_protobuf_wire_format_lite_write_fixed32_to_array},
    {"write_fixed64_to_array", tmc_protobuf_wire_format_lite_write_fixed64_to_array},
    {"write_sfixed32_to_array", tmc_protobuf_wire_format_lite_write_sfixed32_to_array},
    {"write_sfixed64_to_array", tmc_protobuf_wire_format_lite_write_sfixed64_to_array},
    {"write_float_to_array", tmc_protobuf_wire_format_lite_write_float_to_array},
    {"write_double_to_array", tmc_protobuf_wire_format_lite_write_double_to_array},
    {"write_bool_to_array", tmc_protobuf_wire_format_lite_write_bool_to_array},
    {"write_enum_to_array", tmc_protobuf_wire_format_lite_write_enum_to_array},
    {NULL, NULL}};
static const struct constant wire_format_lite_constants[] = {
    {"PARSE", 0},
    {"SERIALIZE", 1},
    {NULL, 0}};

static const struct luaL_Reg internal_metadata_index_methods[] = {
    {"clear", tmc_protobuf_internal_metadata_clear},
    {"have_unknown_fields", tmc_protobuf_internal_metadata_have_unknown_fields},
    {"unknown_fields", tmc_protobuf_internal_metadata_unknown_fields},
    {"set_unknown_fields", tmc_protobuf_internal_metadata_set_unknown_fields},
    {NULL, NULL}};

static const struct luaL_Reg parse_context_index_methods[] = {
    {"done", tmc_protobuf_parse_context_done},
    {"parse_message", tmc_protobuf_parse_context_parse_message},
    {"data_available", tmc_protobuf_parse_context_data_available},
    {NULL, NULL}};

static const struct luaL_Reg eps_copy_output_stream_index_methods[] = {
    {"ensure_space", tmc_protobuf_eps_copy_output_stream_ensure_space},
    {"write_string_maybe_aliased", tmc_protobuf_eps_copy_output_stream_write_string_maybe_aliased},
    {"write_bytes_maybe_aliased", tmc_protobuf_eps_copy_output_stream_write_bytes_maybe_aliased},
    {"write_string", tmc_protobuf_eps_copy_output_stream_write_string},
    {"write_bytes", tmc_protobuf_eps_copy_output_stream_write_bytes},
    {"write_raw", tmc_protobuf_eps_copy_output_stream_write_raw},
    {"write_int32_packed", tmc_protobuf_eps_copy_output_stream_write_int32_packed},
    {"write_uint32_packed", tmc_protobuf_eps_copy_output_stream_write_uint32_packed},
    {"write_sint32_packed", tmc_protobuf_eps_copy_output_stream_write_sint32_packed},
    {"write_int64_packed", tmc_protobuf_eps_copy_output_stream_write_int64_packed},
    {"write_uint64_packed", tmc_protobuf_eps_copy_output_stream_write_uint64_packed},
    {"write_sint64_packed", tmc_protobuf_eps_copy_output_stream_write_sint64_packed},
    {"write_enum_packed", tmc_protobuf_eps_copy_output_stream_write_enum_packed},
    {"write_fixed_packed", tmc_protobuf_eps_copy_output_stream_write_fixed_packed},
    {NULL, NULL}};
static const struct constant eps_copy_output_stream_constants[] = {
    {"INT32", CPPTYPE_INT32},
    {"INT64", CPPTYPE_INT64},
    {"UINT32", CPPTYPE_UINT32},
    {"UINT64", CPPTYPE_UINT64},
    {"DOUBLE", CPPTYPE_DOUBLE},
    {"FLOAT", CPPTYPE_FLOAT},
    {"BOOL", CPPTYPE_BOOL},
    {NULL, 0}};

static void tmc_protobuf_lua_setnullptr(lua_State *L, const char *meta_context)
{
    tmc_protobuf_lua_push_udata(L, nullptr, meta_context);
    lua_setfield(L, -2, "nullptr");
}

static void tmc_protobuf_lua_setintegers(lua_State *L, const struct constant *constants)
{
    for (int i = 0; constants[i].name != NULL; i++)
    {
        tmc_protobuf_lua_push_int(L, constants[i].value);
        lua_setfield(L, -2, constants[i].name);
    }
}

extern "C" int luaopen_tmc_protobuf(lua_State *L)
{
    lua_newtable(L);
    tmc_protobuf_lua_setclass(L, "char", PROTOBUF_CHAR_META_CONTEXT, nullmethod, nullmethod, char_meta_methods, nullconstant);
    tmc_protobuf_lua_setclass(L, "message_lite", PROTOBUF_MESSAGE_LITE_META_CONTEXT, message_lite_methods, message_lite_index_methods, nullmethod, nullconstant);
    tmc_protobuf_lua_setclass(L, "internal", PROTOBUF_INTERNAL_META_CONTEXT, internal_methods, nullmethod, nullmethod, internal_constants);
    tmc_protobuf_lua_setclass(L, "wire_format_lite", PROTOBUF_WIRE_FORMAT_LITE_META_CONTEXT, wire_format_lite_methods, nullmethod, nullmethod, wire_format_lite_constants);
    tmc_protobuf_lua_setclass(L, "internal_metadata", PROTOBUF_INTERNAL_METADATA_META_CONTEXT, nullmethod, internal_metadata_index_methods, nullmethod, nullconstant);
    tmc_protobuf_lua_setclass(L, "parse_context", PROTOBUF_PARSE_CONTEXT_META_CONTEXT, nullmethod, parse_context_index_methods, nullmethod, nullconstant);
    tmc_protobuf_lua_setclass(L, "eps_copy_output_stream", PROTOBUF_EPS_COPY_OUTPUT_STREAM_META_CONTEXT, nullmethod, eps_copy_output_stream_index_methods, nullmethod, eps_copy_output_stream_constants);
    tmc_protobuf_lua_setclass(L, "uint8", PROTOBUF_UINT8_META_CONTEXT, nullmethod, nullmethod, nullmethod, nullconstant);
    return 1;
}
