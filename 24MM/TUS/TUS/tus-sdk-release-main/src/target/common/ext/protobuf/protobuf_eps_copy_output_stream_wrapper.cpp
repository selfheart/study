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
/*
This is a wrapper module for Lua to wrap Protocol Buffers.
<br>
Wrapper functions to use Protocol Buffers is implemented in this module.
However, this module is internal module which a user dose not use directly.
protoc Lua plugin generates abstract wrapper of Lua scripts that a user use directly.
To know the protobuf Lua library usage, please see protoc Lua plugin.
@module tmc_protobuf
*/
#include "protobuf_eps_copy_output_stream_wrapper.hpp"

#include "protobuf_helper.hpp"

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/port.h>
#include <google/protobuf/repeated_field.h>

// shall be noexcept-safe
#define tmc_lua_check_repeated_field(L, ARG, REPEATED_FIELD, TYPE) \
    do                                                             \
    {                                                              \
        tmc_protobuf_lua_check_table(L, ARG);                      \
        lua_pushnil(L);                                            \
        while (lua_next(L, ARG))                                   \
        {                                                          \
            TYPE i = tmc_protobuf_lua_check_##TYPE(L, -1);         \
            try {                                                  \
                (REPEATED_FIELD).Add(i);                           \
                lua_pop(L, 1);                                     \
            } catch (...) { return tmc_protobuf_lua_error(L, __func__); } \
        }                                                          \
    } while (0)

// shall be noexcept-safe
#define write_packed(L, WRITE_FUNC, TYPE)                                                \
    do                                                                                   \
    {                                                                                    \
        EpsCopyOutputStream *self = tmc_protobuf_lua_check_eps_copy_output_stream(L, 1); \
        int num = tmc_protobuf_lua_check_int(L, 2);                                      \
        RepeatedField<TYPE> r;                                                           \
        tmc_lua_check_repeated_field(L, 3, r, TYPE);                                     \
        int size = tmc_protobuf_lua_check_int(L, 4);                                     \
        uint8 *ptr = tmc_protobuf_lua_check_uint8_ptr(L, 5);                             \
        try {                                                                            \
            ptr = self->WRITE_FUNC(num, r, size, ptr);                                   \
        } catch (...) { return tmc_protobuf_lua_error(L, __func__); }                    \
        tmc_protobuf_lua_push_uint8_ptr(L, ptr);                                         \
    } while (0)

/*
 * fixme: google::protobuf::* may throw some exceptions.
 * for now, use try {} catch (...) {return tmc_protobuf_lua_error(L, __func__)}
 * to avoid triggering an abnormal termination.
 */
using google::protobuf::int32;
using google::protobuf::int64;
using google::protobuf::RepeatedField;
using google::protobuf::uint32;
using google::protobuf::uint64;
using google::protobuf::uint8;
using google::protobuf::io::EpsCopyOutputStream;

static int write_string(lua_State *L, uint8 *(EpsCopyOutputStream::*write_func)(uint32, const std::string &, uint8 *))
    noexcept
{
    EpsCopyOutputStream *self = tmc_protobuf_lua_check_eps_copy_output_stream(L, 1);
    uint32 num = tmc_protobuf_lua_check_uint32(L, 2);
    size_t len = 0;
    const char *s = luaL_checklstring(L, 3, &len);
    uint8 *ptr = tmc_protobuf_lua_check_uint8_ptr(L, 4);
    try {
        const std::string str(s, len);
        ptr = (self->*write_func)(num, str, ptr);
    } catch (...) { return tmc_protobuf_lua_error(L, __func__); }
    tmc_protobuf_lua_push_uint8_ptr(L, ptr);
    return 1;
}

/*
Wrapper of google::protobuf::io::EpsCopyOutputStream::EnsureSpace.
<pre><code>
google::protobuf::uint8 *
    google::protobuf::io::EpsCopyOutputStream::EnsureSpace(
        google::protobuf::uint8 *ptr)
</code></pre>
After this, it's guaranteed you can safely write kSlopBytes to ptr.
This will never fail.
The underlying stream can produce an error.
Use HadError to check for errors.
@function eps_copy_output_stream.ensure_space(ptr)
@tparam uint8* ptr
@treturn uint8* return value of the wrapped function
@usage ptr = stream:ensure_space(ptr)
*/
int tmc_protobuf_eps_copy_output_stream_ensure_space(lua_State *L)
    noexcept
{
    EpsCopyOutputStream *self = tmc_protobuf_lua_check_eps_copy_output_stream(L, 1);
    uint8 *ptr = tmc_protobuf_lua_check_uint8_ptr(L, 2);
    try {
        ptr = self->EnsureSpace(ptr);
    } catch (...) { return tmc_protobuf_lua_error(L, __func__); }
    tmc_protobuf_lua_push_uint8_ptr(L, ptr);
    return 1;
}

/*
Wrapper of google::protobuf::io::EpsCopyOutputStream::WriteStringMaybeAliased.
<pre><code>
google::protobuf::uint8 *
    google::protobuf::io::EpsCopyOutputStream::WriteStringMaybeAliased(
        google::protobuf::uint32 num,
        const std::string &s,
        google::protobuf::uint8 *ptr)
</code></pre>
@function eps_copy_output_stream.write_string_maybe_aliased(num, s, ptr)
@tparam num num
@tparam string s
@tparam uint8* ptr
@treturn uint8* return value of the wrapped function
@usage ptr = stream:write_string_maybe_aliased(num, s, ptr)
*/
int tmc_protobuf_eps_copy_output_stream_write_string_maybe_aliased(lua_State *L)
    noexcept
{
    return write_string(L, &EpsCopyOutputStream::WriteStringMaybeAliased);
}

/*
Wrapper of google::protobuf::io::EpsCopyOutputStream::WriteBytesMaybeAliased.
<pre><code>
google::protobuf::uint8 *
    google::protobuf::io::EpsCopyOutputStream::WriteBytesMaybeAliased(
        google::protobuf::uint32 num,
        const std::string &s,
        google::protobuf::uint8 *ptr)
</code></pre>
@function eps_copy_output_stream.write_bytes_maybe_aliased(num, s, ptr)
@tparam num num
@tparam string s
@tparam uint8* ptr
@treturn uint8* return value of the wrapped function
@usage ptr = stream:write_bytes_maybe_aliased(num, s, ptr)
*/
int tmc_protobuf_eps_copy_output_stream_write_bytes_maybe_aliased(lua_State *L)
    noexcept
{
    return write_string(L, &EpsCopyOutputStream::WriteBytesMaybeAliased);
}

/*
Wrapper of google::protobuf::io::EpsCopyOutputStream::WriteString.
<pre><code>
google::protobuf::uint8 *
    google::protobuf::io::EpsCopyOutputStream::WriteString(
        google::protobuf::uint32 num,
        const std::string &s,
        google::protobuf::uint8 *ptr)
</code></pre>
@function eps_copy_output_stream.write_string(num, s, ptr)
@tparam num num
@tparam string s
@tparam uint8* ptr
@treturn uint8* return value of the wrapped function
@usage ptr = stream:write_string(num, s, ptr)
*/
int tmc_protobuf_eps_copy_output_stream_write_string(lua_State *L)
    noexcept
{
    return write_string(L, &EpsCopyOutputStream::WriteString);
}

/*
Wrapper of google::protobuf::io::EpsCopyOutputStream::WriteBytes.
<pre><code>
google::protobuf::uint8 *
    google::protobuf::io::EpsCopyOutputStream::WriteBytes(
        google::protobuf::uint32 num,
        const std::string &s,
        google::protobuf::uint8 *ptr)
</code></pre>
@function write_bytes(num, s, ptr)
@tparam num num
@tparam string s
@tparam uint8* ptr
@treturn uint8* return value of the wrapped function
@usage ptr = stream:write_bytes(num, s, ptr)
*/
int tmc_protobuf_eps_copy_output_stream_write_bytes(lua_State *L)
    noexcept
{
    return write_string(L, &EpsCopyOutputStream::WriteBytes);
}

/*
Wrapper of google::protobuf::io::EpsCopyOutputStream::WriteRaw.
<pre><code>
google::protobuf::uint8 *
    google::protobuf::io::EpsCopyOutputStream::WriteRaw(
        const void *data,
        int size,
        google::protobuf::uint8 *ptr)
</code></pre>
@function eps_copy_output_stream.write_raw(data, size, ptr)
@tparam string data
@tparam integer size
@tparam uint8* ptr
@treturn uint8* return value of the wrapped function
@usage ptr = stream:write_raw(data, size, ptr)
*/
int tmc_protobuf_eps_copy_output_stream_write_raw(lua_State *L)
    noexcept
{
    EpsCopyOutputStream *self = tmc_protobuf_lua_check_eps_copy_output_stream(L, 1);
    const char *data = luaL_checkstring(L, 2);
    int size = tmc_protobuf_lua_check_int(L, 3);
    uint8 *ptr = tmc_protobuf_lua_check_uint8_ptr(L, 4);
    try {
        ptr = self->WriteRaw(data, size, ptr);
    } catch (...) { return tmc_protobuf_lua_error(L, __func__); }
    tmc_protobuf_lua_push_uint8_ptr(L, ptr);
    return 1;
}

/*
Wrapper of google::protobuf::io::EpsCopyOutputStream::WriteInt32Packed.
<pre><code>
template<class T> google::protobuf::uint8 *
    google::protobuf::io::EpsCopyOutputStream::WriteInt32Packed(
        int num,
        const T &r,
        int size,
        google::protobuf::uint8 *ptr)
</code></pre>
@function eps_copy_output_stream.write_int32_packed(num, r, size, ptr)
@tparam integer num
@tparam integer r
@tparam integer size
@tparam uint8* ptr
@treturn uint8* return value of the wrapped function
@usage ptr = stream:write_int32_packed(num, r, size, ptr)
*/
int tmc_protobuf_eps_copy_output_stream_write_int32_packed(lua_State *L)
    noexcept
{
    write_packed(L, WriteInt32Packed, int32);
    return 1;
}

/*
Wrapper of google::protobuf::io::EpsCopyOutputStream::WriteUInt32Packed.
<pre><code>
template<class T> google::protobuf::uint8 *
    google::protobuf::io::EpsCopyOutputStream::WriteUInt32Packed(
        int num,
        const T &r,
        int size,
        google::protobuf::uint8 *ptr)
</code></pre>
@function eps_copy_output_stream.write_uint32_packed(num, r, size, ptr)
@tparam integer num
@tparam integer r
@tparam integer size
@tparam uint8* ptr
@treturn uint8* return value of the wrapped function
@usage ptr = stream:write_uint32_packed(num, r, size, ptr)
*/
int tmc_protobuf_eps_copy_output_stream_write_uint32_packed(lua_State *L)
    noexcept
{
    write_packed(L, WriteUInt32Packed, uint32);
    return 1;
}

/*
Wrapper of google::protobuf::io::EpsCopyOutputStream::WriteSInt32Packed.
<pre><code>
template<class T> google::protobuf::uint8 *
    google::protobuf::io::EpsCopyOutputStream::WriteSInt32Packed(
        int num, const T &r,
        int size,
        google::protobuf::uint8 *ptr)
</code></pre>
@function eps_copy_output_stream.write_sint32_packed(num, r, size, ptr)
@tparam integer num
@tparam integer r
@tparam integer size
@tparam uint8* ptr
@treturn uint8* return value of the wrapped function
@usage ptr = stream:write_sint32_packed(num, r, size, ptr)
*/
int tmc_protobuf_eps_copy_output_stream_write_sint32_packed(lua_State *L)
    noexcept
{
    write_packed(L, WriteSInt32Packed, int32);
    return 1;
}

/*
Wrapper of google::protobuf::io::EpsCopyOutputStream::WriteInt64Packed.
<pre><code>
template<class T> google::protobuf::uint8 *
    google::protobuf::io::EpsCopyOutputStream::WriteInt64Packed(
        int num,
        const T &r,
        int size,
        google::protobuf::uint8 *ptr)
</code></pre>
@function eps_copy_output_stream.write_int64_packed(num, r, size, ptr)
@tparam integer num
@tparam integer r
@tparam integer size
@tparam uint8* ptr
@treturn uint8* return value of the wrapped function
@usage ptr = stream:write_int64_packed(num, r, size, ptr)
*/
int tmc_protobuf_eps_copy_output_stream_write_int64_packed(lua_State *L)
    noexcept
{
    write_packed(L, WriteInt64Packed, int64);
    return 1;
}

/*
Wrapper of google::protobuf::io::EpsCopyOutputStream::WriteUInt64Packed.
<pre><code>
template<class T> google::protobuf::uint8 *
    google::protobuf::io::EpsCopyOutputStream::WriteUInt64Packed(
        int num,
        const T &r,
        int size,
        google::protobuf::uint8 *ptr)
</code></pre>
@function eps_copy_output_stream.write_uint64_packed(num, r, size, ptr)
@tparam integer num
@tparam integer r
@tparam integer size
@tparam uint8* ptr
@treturn uint8* return value of the wrapped function
@usage ptr = stream:write_uint64_packed(num, r, size, ptr)
*/
int tmc_protobuf_eps_copy_output_stream_write_uint64_packed(lua_State *L)
    noexcept
{
    write_packed(L, WriteUInt64Packed, uint64);
    return 1;
}

/*
Wrapper of google::protobuf::io::EpsCopyOutputStream::WriteSInt64Packed.
<pre><code>
template<class T> google::protobuf::uint8 *
    google::protobuf::io::EpsCopyOutputStream::WriteSInt64Packed(
        int num,
        const T &r,
        int size,
        google::protobuf::uint8 *ptr)
</code></pre>
@function eps_copy_output_stream.write_sint64_packed(num, r, size, ptr)
@tparam integer num
@tparam integer r
@tparam integer size
@tparam uint8* ptr
@treturn uint8* return value of the wrapped function
@usage ptr = stream:write_sint64_packed(num, r, size, ptr)
*/
int tmc_protobuf_eps_copy_output_stream_write_sint64_packed(lua_State *L)
    noexcept
{
    write_packed(L, WriteSInt64Packed, int64);
    return 1;
}

/*
Wrapper of google::protobuf::io::EpsCopyOutputStream::WriteEnumPacked.
<pre><code>
template<class T> google::protobuf::uint8 *
    google::protobuf::io::EpsCopyOutputStream::WriteEnumPacked(
        int num,
        const T &r,
        int size,
        google::protobuf::uint8 *ptr)
</code></pre>
@function write_enum_packed(num, r, size, ptr)
@tparam integer num
@tparam integer r
@tparam integer size
@tparam uint8* ptr
@treturn uint8* return value of the wrapped function
@usage ptr = stream:write_enum_packed(num, r, size, ptr)
*/
int tmc_protobuf_eps_copy_output_stream_write_enum_packed(lua_State *L)
    noexcept
{
    write_packed(L, WriteEnumPacked, int64);
    return 1;
}

/*
Wrapper of google::protobuf::io::EpsCopyOutputStream::WriteFixedPacked.
<pre><code>
template<class T> google::protobuf::uint8 *
    google::protobuf::io::EpsCopyOutputStream::WriteFixedPacked(
        int num,
        const T &r,
        google::protobuf::uint8 *ptr)
</code></pre>
@function eps_copy_output_stream.write_fixed_packed(num, r, ptr, type_num)
@tparam integer num
@tparam integer/number r
@tparam uint8 ptr
@tparam integer type_num
@treturn uint8* return value of the wrapped function
@usage ptr = stream:write_enum_packed(num, r, ptr, type_num)
*/
int tmc_protobuf_eps_copy_output_stream_write_fixed_packed(lua_State *L)
    noexcept
{
    EpsCopyOutputStream *self = tmc_protobuf_lua_check_eps_copy_output_stream(L, 1);
    int num = tmc_protobuf_lua_check_int(L, 2);
    uint8 *ptr = tmc_protobuf_lua_check_uint8_ptr(L, 4);
    int type_num = tmc_protobuf_lua_check_int(L, 5);
    switch (type_num)
    {

#define CASE(TYPE_CONST, TYPE)                       \
    case TYPE_CONST:                                 \
    {                                                \
        RepeatedField<TYPE> r;                       \
        tmc_lua_check_repeated_field(L, 3, r, TYPE); \
        ptr = self->WriteFixedPacked(num, r, ptr);   \
        break;                                       \
    }

        CASE(CPPTYPE_INT32, int32)
        CASE(CPPTYPE_INT64, int64)
        CASE(CPPTYPE_UINT32, uint32)
        CASE(CPPTYPE_UINT64, uint64)
        CASE(CPPTYPE_DOUBLE, double)
        CASE(CPPTYPE_FLOAT, float)
        CASE(CPPTYPE_BOOL, bool)

#undef CASE

    default:
        return tmc_protobuf_lua_error(L, "Invalid type '%d'.\n", type_num);
    }
    tmc_protobuf_lua_push_uint8_ptr(L, ptr);
    return 1;
}
