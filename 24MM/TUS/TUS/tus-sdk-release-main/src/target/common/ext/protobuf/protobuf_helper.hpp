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
#ifndef TMC_PROTOBUF_HELPER_H__
#define TMC_PROTOBUF_HELPER_H__

#include <lua.hpp>
#include <cstdint>
#include <climits>

#include "protobuf_meta_context.hpp"
#include "protobuf_message_lite.hpp"

enum CppType
{
    CPPTYPE_INT32 = 1,    // TYPE_INT32, TYPE_SINT32, TYPE_SFIXED32
    CPPTYPE_INT64 = 2,    // TYPE_INT64, TYPE_SINT64, TYPE_SFIXED64
    CPPTYPE_UINT32 = 3,   // TYPE_UINT32, TYPE_FIXED32
    CPPTYPE_UINT64 = 4,   // TYPE_UINT64, TYPE_FIXED64
    CPPTYPE_DOUBLE = 5,   // TYPE_DOUBLE
    CPPTYPE_FLOAT = 6,    // TYPE_FLOAT
    CPPTYPE_BOOL = 7,     // TYPE_BOOL
    CPPTYPE_ENUM = 8,     // TYPE_ENUM
    CPPTYPE_STRING = 9,   // TYPE_STRING, TYPE_BYTES
    CPPTYPE_MESSAGE = 10, // TYPE_MESSAGE, TYPE_GROUP
};

namespace pb = google::protobuf;

void tmc_protobuf_lua_push_udata(lua_State *L, const void *udata, const char *metacontext);
void tmc_protobuf_lua_push_message_lite(lua_State *L, const MessageLiteLua *message_lite);

inline void tmc_protobuf_lua_push_int32(lua_State *L, int32_t n) { lua_pushinteger(L, n); }
inline void tmc_protobuf_lua_push_uint32(lua_State *L, uint32_t n) { lua_pushinteger(L, n); }
inline void tmc_protobuf_lua_push_int64(lua_State *L, int64_t n) { lua_pushinteger(L, n); }
inline void tmc_protobuf_lua_push_uint64(lua_State *L, uint64_t n)
{
    if (n > LLONG_MAX) {
        try {
            lua_pushstring(L, std::to_string(n).c_str());
        } catch (...) { lua_error(L); /* noreturn */ }
    } else
        lua_pushinteger(L, (lua_Integer)n);
}
inline void tmc_protobuf_lua_push_int(lua_State *L, int n) { lua_pushinteger(L, n); }
inline void tmc_protobuf_lua_push_size_t(lua_State *L, size_t n) { lua_pushinteger(L, n); }
inline void tmc_protobuf_lua_push_double(lua_State *L, double n) { lua_pushnumber(L, n); }
inline void tmc_protobuf_lua_push_float(lua_State *L, float n) { lua_pushnumber(L, n); }
inline void tmc_protobuf_lua_push_bool(lua_State *L, bool b) { lua_pushboolean(L, (b) ? 1 : 0); };

inline void tmc_protobuf_lua_push_char_ptr(lua_State *L, const char *udata)
{
    tmc_protobuf_lua_push_udata(L, udata, PROTOBUF_CHAR_META_CONTEXT);
}
inline void tmc_protobuf_lua_push_uint8_ptr(lua_State *L, const pb::uint8 *udata)
{
    tmc_protobuf_lua_push_udata(L, udata, PROTOBUF_UINT8_META_CONTEXT);
}
inline void tmc_protobuf_lua_push_internal_metadata(lua_State *L, const pb::internal::InternalMetadata *udata)
{
    tmc_protobuf_lua_push_udata(L, udata, PROTOBUF_INTERNAL_METADATA_META_CONTEXT);
}
inline void tmc_protobuf_lua_push_parse_context(lua_State *L, const pb::internal::ParseContext *udata)
{
    tmc_protobuf_lua_push_udata(L, udata, PROTOBUF_PARSE_CONTEXT_META_CONTEXT);
}
inline void tmc_protobuf_lua_push_eps_copy_output_stream(lua_State *L, const pb::io::EpsCopyOutputStream *udata)
{
    tmc_protobuf_lua_push_udata(L, udata, PROTOBUF_EPS_COPY_OUTPUT_STREAM_META_CONTEXT);
}

void *tmc_protobuf_lua_check_udata(lua_State *L, int arg, const char *tname);
void *tmc_protobuf_lua_check_udata_ptr_ptr(lua_State *L, int arg, const char *tname);
MessageLiteLua *tmc_protobuf_lua_check_message_lite(lua_State *L, int arg);
MessageLiteLua **tmc_protobuf_lua_check_message_lite_ptr_ptr(lua_State *L, int arg);
int tmc_protobuf_lua_check_int(lua_State *L, int arg);
size_t tmc_protobuf_lua_check_size_t(lua_State *L, int arg);
int32_t tmc_protobuf_lua_check_int32(lua_State *L, int arg);
uint32_t tmc_protobuf_lua_check_uint32(lua_State *L, int arg);
int64_t tmc_protobuf_lua_check_int64(lua_State *L, int arg);
uint64_t tmc_protobuf_lua_check_uint64(lua_State *L, int arg);
double tmc_protobuf_lua_check_double(lua_State *L, int arg);
float tmc_protobuf_lua_check_float(lua_State *L, int arg);
bool tmc_protobuf_lua_check_bool(lua_State *L, int arg);

inline char *tmc_protobuf_lua_check_char_ptr(lua_State *L, int arg)
{
    return (char *)tmc_protobuf_lua_check_udata(L, arg, PROTOBUF_CHAR_META_CONTEXT);
}
inline char **tmc_protobuf_lua_check_char_ptr_ptr(lua_State *L, int arg)
{
    return (char **)tmc_protobuf_lua_check_udata_ptr_ptr(L, arg, PROTOBUF_CHAR_META_CONTEXT);
}
inline pb::io::EpsCopyOutputStream *tmc_protobuf_lua_check_eps_copy_output_stream(lua_State *L, int arg)
{
    return (pb::io::EpsCopyOutputStream *)tmc_protobuf_lua_check_udata(L, arg, PROTOBUF_EPS_COPY_OUTPUT_STREAM_META_CONTEXT);
}
inline pb::uint8 *tmc_protobuf_lua_check_uint8_ptr(lua_State *L, int arg)
{
    return (pb::uint8 *)tmc_protobuf_lua_check_udata(L, arg, PROTOBUF_UINT8_META_CONTEXT);
}
inline pb::internal::InternalMetadata *tmc_protobuf_lua_check_internal_metadata(lua_State *L, int arg)
{
    return (pb::internal::InternalMetadata *)tmc_protobuf_lua_check_udata(L, arg, PROTOBUF_INTERNAL_METADATA_META_CONTEXT);
}
inline pb::internal::ParseContext *tmc_protobuf_lua_check_parse_context(lua_State *L, int arg)
{
    return (pb::internal::ParseContext *)tmc_protobuf_lua_check_udata(L, arg, PROTOBUF_PARSE_CONTEXT_META_CONTEXT);
}

int tmc_protobuf_lua_error(lua_State *L, const char *fmt, ...);

long long tmc_protobuf_lua_check_integer(lua_State *L, int arg);
void tmc_protobuf_lua_check_table(lua_State *L, int arg);

#endif // TMC_PROTOBUF_HELPER_H__
