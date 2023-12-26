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
@module tmc_protobuf
*/
#include "protobuf_wire_format_lite_wrapper.hpp"

#include "protobuf_message_lite.hpp"
#include "protobuf_helper.hpp"

#include <string>

#include <google/protobuf/wire_format_lite.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/stubs/port.h>
#include <google/protobuf/unknown_field_set.h>
#include <google/protobuf/repeated_field.h>

// shall be noexcept-safe
#define val_size(L, SIZE_FUNC, TYPE)                                                                 \
    do                                                                                               \
    {                                                                                                \
        size_t size = 0;                                                                             \
        if (lua_isstring(L, 1))                                                                      \
        {                                                                                            \
            TYPE value = tmc_protobuf_lua_check_##TYPE(L, 1);                                        \
            size = SIZE_FUNC(value);                                                                 \
        }                                                                                            \
        else if (lua_istable(L, 1))                                                                  \
        {                                                                                            \
            tmc_protobuf_lua_check_table(L, 1);                                                      \
            RepeatedField<TYPE> value;                                                               \
            lua_pushnil(L);                                                                          \
            while (lua_next(L, 1))                                                                   \
            {                                                                                        \
                TYPE i = tmc_protobuf_lua_check_##TYPE(L, -1);                                       \
                try {                                                                                \
                    value.Add(i);                                                                    \
                } catch (...) { return tmc_protobuf_lua_error(L, __func__); }                        \
                lua_pop(L, 1);                                                                       \
            }                                                                                        \
            try {                                                                                    \
                size = SIZE_FUNC(value);                                                             \
            } catch (...) { return tmc_protobuf_lua_error(L, __func__); }                            \
        }                                                                                            \
        else                                                                                         \
        {                                                                                            \
            tmc_protobuf_lua_error(L, "(%s) Invalid type '%s'.\n", #SIZE_FUNC, luaL_typename(L, 1)); \
        }                                                                                            \
        tmc_protobuf_lua_push_size_t(L, size);                                                       \
    } while (0)

// shall be noexcept-safe
#define write_val_to_array(L, WRITE_FUNC, TYPE)                                                       \
    do                                                                                                \
    {                                                                                                 \
        int field_number = tmc_protobuf_lua_check_int(L, 1);                                          \
        uint8 *target = tmc_protobuf_lua_check_uint8_ptr(L, 3);                                       \
        if (lua_isstring(L, 2) || lua_isboolean(L, 2))                                                \
        {                                                                                             \
            TYPE value = tmc_protobuf_lua_check_##TYPE(L, 2);                                         \
            target = WRITE_FUNC(field_number, value, target);                                         \
        }                                                                                             \
        else if (lua_istable(L, 2))                                                                   \
        {                                                                                             \
            tmc_protobuf_lua_check_table(L, 2);                                                       \
            RepeatedField<TYPE> value;                                                                \
            lua_pushnil(L);                                                                           \
            while (lua_next(L, 2))                                                                    \
            {                                                                                         \
                TYPE i = tmc_protobuf_lua_check_##TYPE(L, -1);                                        \
                try {                                                                                 \
                    value.Add(i);                                                                     \
                } catch (...) { return tmc_protobuf_lua_error(L, __func__); }                         \
                lua_pop(L, 1);                                                                        \
            }                                                                                         \
            try {                                                                                     \
                target = WRITE_FUNC(field_number, value, target);                                     \
            } catch (...) { return tmc_protobuf_lua_error(L, __func__); }                             \
        }                                                                                             \
        else                                                                                          \
        {                                                                                             \
            tmc_protobuf_lua_error(L, "(%s) Invalid type '%s'.\n", #WRITE_FUNC, luaL_typename(L, 2)); \
        }                                                                                             \
        tmc_protobuf_lua_push_uint8_ptr(L, target);                                                   \
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
using google::protobuf::UnknownFieldSet;
using google::protobuf::internal::WireFormatLite;
using google::protobuf::io::EpsCopyOutputStream;

static int string_size(lua_State *L, size_t (*size_func)(const std::string &value))
    noexcept
{
    size_t len = 0;
    const char *s = luaL_checklstring(L, 1, &len);
    size_t size;
    try {
        std::string value(s, len);
        size = (*size_func)(value);
    } catch (...) { return tmc_protobuf_lua_error(L, __func__); }
    tmc_protobuf_lua_push_size_t(L, size);
    return 1;
}

/*
Wrapper of google::protobuf::internal::WireFormatLite::VerifyUtf8String.
<pre><code>
static bool google::protobuf::internal::WireFormatLite::VerifyUtf8String(
    const char *data,
    int size,
    google::protobuf::internal::WireFormatLite::Operation op,
    const char *field_name)
</code></pre>
Returns true if the data is valid UTF-8.
@function wire_format_lite.verify_utf8_string(data, size, op, field_name)
@tparam string data
@tparam integer size
@tparam integer op
@tparam string field_name
@treturn boolean return value of the wrapped function
@usage ret = protobuf.wire_format_lite.verify_utf8_string(message.name, message.name:len(), pb.wire_format_lite.SERIALIZE, "user_datasets.User.name")
*/
int tmc_protobuf_wire_format_lite_verify_utf8_string(lua_State *L)
    noexcept
{
    const char *data = luaL_checkstring(L, 1);
    int size = tmc_protobuf_lua_check_int(L, 2);
    WireFormatLite::Operation op = (WireFormatLite::Operation)tmc_protobuf_lua_check_int(L, 3);
    const char *field_name = luaL_checkstring(L, 4);
    bool b;
    try {
        b = WireFormatLite::VerifyUtf8String(data, size, op, field_name);
    } catch (...) { return tmc_protobuf_lua_error(L, __func__); }
    tmc_protobuf_lua_push_bool(L, b);
    return 1;
}

/*
Wrapper of google::protobuf::internal::WireFormatLite::InternalWriteMessage.
<pre><code>
template <typename MessageType> static google::protobuf::uint8 *
    google::protobuf::internal::WireFormatLite::InternalWriteMessage(
        int field_number,
        const MessageType &value,
        google::protobuf::uint8 *target,
        google::protobuf::io::EpsCopyOutputStream *stream)
</code></pre>
@function internal_write_message(field_number, value, target, stream)
@tparam integer field_number
@tparam MessageLite value
@tparam uint8* target
@tparam EpsCopyOutputStream stream
@treturn uint8* return value of the wrapped function
@usage target = pb.wire_format_lite.internal_write_message(1, message.message, target, stream)
*/
int tmc_protobuf_wire_format_lite_internal_write_message(lua_State *L)
    noexcept
{
    int field_number = tmc_protobuf_lua_check_int(L, 1);
    MessageLiteLua *value = tmc_protobuf_lua_check_message_lite(L, 2);
    uint8 *target = tmc_protobuf_lua_check_uint8_ptr(L, 3);
    EpsCopyOutputStream *stream = tmc_protobuf_lua_check_eps_copy_output_stream(L, 4);
    try {
        target = WireFormatLite::InternalWriteMessage(field_number, *value, target, stream);
    } catch (...) { return tmc_protobuf_lua_error(L, __func__); }
    tmc_protobuf_lua_push_uint8_ptr(L, target);
    return 1;
}

/*
Wrapper of google::protobuf::internal::WireFormatLite::Int32Size.
<pre><code>
size_t google::protobuf::internal::WireFormatLite::Int32Size(google::protobuf::int32 value)
</code></pre>
Compute the byte size of a field.
The XxSize() functions do NOT include the tag, so you must also call TagSize().
(This is because, for repeated fields, you should only call TagSize() once and multiply it by the element count, but you may have to call XxSize() for each individual element.)
@function wire_format_lite.int32_size(value)
@tparam integer value
@treturn integer return value of the wrapped function
@usage size = protobuf.wire_format_lite.int32_size(value)
*/
int tmc_protobuf_wire_format_lite_int32_size(lua_State *L)
    noexcept
{
    val_size(L, WireFormatLite::Int32Size, int32);
    return 1;
}

/*
Wrapper of google::protobuf::internal::WireFormatLite::Int64Size.
<pre><code>
size_t google::protobuf::internal::WireFormatLite::Int64Size(google::protobuf::int64 value)
</code></pre>
@function wire_format_lite.int64_size(value)
@tparam integer value
@treturn integer return value of the wrapped function
@usage size = protobuf.wire_format_lite.int64_size(value)
*/
int tmc_protobuf_wire_format_lite_int64_size(lua_State *L)
    noexcept
{
    val_size(L, WireFormatLite::Int64Size, int64);
    return 1;
}

/*
Wrapper of google::protobuf::internal::WireFormatLite::UInt32Size.
<pre><code>
size_t google::protobuf::internal::WireFormatLite::UInt32Size(google::protobuf::uint32 value)
</code></pre>
@function wire_format_lite.uint32_size(value)
@tparam integer value
@treturn integer return value of the wrapped function
@usage size = protobuf.wire_format_lite.uint32_size(value)
*/
int tmc_protobuf_wire_format_lite_uint32_size(lua_State *L)
    noexcept
{
    val_size(L, WireFormatLite::UInt32Size, uint32);
    return 1;
}

/*
Wrapper of google::protobuf::internal::WireFormatLite::UInt64Size.
<pre><code>
size_t google::protobuf::internal::WireFormatLite::UInt64Size(google::protobuf::uint64 value)
</code></pre>
@function wire_format_lite.uint64_size(value)
@tparam integer value
@treturn integer return value of the wrapped function
@usage size = protobuf.wire_format_lite.uint64_size(value)
*/
int tmc_protobuf_wire_format_lite_uint64_size(lua_State *L)
    noexcept
{
    val_size(L, WireFormatLite::UInt64Size, uint64);
    return 1;
}

/*
Wrapper of google::protobuf::internal::WireFormatLite::SUInt32Size.
<pre><code>
size_t google::protobuf::internal::WireFormatLite::SInt32Size(google::protobuf::sint32 value)
</code></pre>
@function wire_format_lite.sint32_size(value)
@tparam integer value
@treturn integer return value of the wrapped function
@usage size = protobuf.wire_format_lite.sint32_size(value)
*/
int tmc_protobuf_wire_format_lite_sint32_size(lua_State *L)
    noexcept
{
    val_size(L, WireFormatLite::SInt32Size, int32);
    return 1;
}

/*
Wrapper of google::protobuf::internal::WireFormatLite::SInt64Size.
<pre><code>
size_t google::protobuf::internal::WireFormatLite::SInt64Size(google::protobuf::sint64 value)
</code></pre>
@function sint64_size(value)
@tparam integer value
@treturn integer return value of the wrapped function
@usage size = protobuf.wire_format_lite.sint64_size(value)
*/
int tmc_protobuf_wire_format_lite_sint64_size(lua_State *L)
    noexcept
{
    val_size(L, WireFormatLite::SInt64Size, int64);
    return 1;
}

/*
Wrapper of google::protobuf::internal::WireFormatLite::EnumSize.
<pre><code>
size_t google::protobuf::internal::WireFormatLite::EnumSize(int value)
</code></pre>
@function wire_format_lite.enum_size(value)
@tparam integer value
@treturn integer return value of the wrapped function
@usage size = protobuf.wire_format_lite.enum_size(value)
*/
int tmc_protobuf_wire_format_lite_enum_size(lua_State *L)
    noexcept
{
    val_size(L, WireFormatLite::EnumSize, int);
    return 1;
}

/*
Wrapper of google::protobuf::internal::WireFormatLite::StringSize.
<pre><code>
static size_t google::protobuf::internal::WireFormatLite::StringSize(const std::string &value)
</code></pre>
@function wire_format_lite.string_size(value)
@tparam string value
@treturn integer return value of the wrapped function
@usage size = protobuf.wire_format_lite.string_size(value)
*/
int tmc_protobuf_wire_format_lite_string_size(lua_State *L)
    noexcept
{
    return string_size(L, WireFormatLite::StringSize);
}

/*
Wrapper of google::protobuf::internal::WireFormatLite::ByteSize.
<pre><code>
static size_t google::protobuf::internal::WireFormatLite::ByteSize(const std::string &value)
</code></pre>
@function wire_format_lite.byte_size(value)
@tparam string value
@treturn integer return value of the wrapped function
@usage size = protobuf.wire_format_lite.byte_size(value)
*/
int tmc_protobuf_wire_format_lite_bytes_size(lua_State *L)
    noexcept
{
    return string_size(L, WireFormatLite::BytesSize);
}

/*
Wrapper of google::protobuf::internal::WireFormatLite::MessageSize.
<pre><code>
static size_t google::protobuf::internal::WireFormatLite::MessageSize<MessageLiteLua>(const MessageLiteLua &value)
</code></pre>
@function wire_format_lite.message_size(value)
@tparam MessageLite value
@treturn integer return value of the wrapped function
@usage size = protobuf.wire_format_lite.message_size(value)
*/
int tmc_protobuf_wire_format_lite_message_size(lua_State *L)
    noexcept
{
    MessageLiteLua *value = tmc_protobuf_lua_check_message_lite(L, 1);
    size_t size = WireFormatLite::MessageSize(*value);
    tmc_protobuf_lua_push_size_t(L, size);
    return 1;
}

/*
Wrapper of google::protobuf::internal::WireFormatLite::WriteInt32ToArray.
<pre><code>
google::protobuf::uint8 *
    google::protobuf::internal::WireFormatLite::WriteInt32ToArray(
        int field_number,
        google::protobuf::int32 value,
        google::protobuf::uint8* target)
google::protobuf::uint8 *
    google::protobuf::internal::WireFormatLite::WriteInt32ToArray(
        int field_number,
        const google::protobuf::RepeatedField<google::protobuf::int32>& value,
        google::protobuf::uint8* target)
</code></pre>
Write a singular field or repeated field to buffer.
@function wire_format_lite.int32_to_array(field_number, value, target)
@tparam integer field_number
@tparam integer/table value
@tparam uint8* target
@treturn uint8* return value of the wrapped function
@usage target = protobuf.wire_format_lite.int32_to_array(number, value, target)
*/
int tmc_protobuf_wire_format_lite_write_int32_to_array(lua_State *L)
    noexcept
{
    write_val_to_array(L, WireFormatLite::WriteInt32ToArray, int32);
    return 1;
}

/*
Wrapper of google::protobuf::internal::WireFormatLite::WriteInt64ToArray.
<pre><code>
google::protobuf::uint8 *
    google::protobuf::internal::WireFormatLite::WriteInt64ToArray(
        int field_number,
        google::protobuf::int64 value,
        google::protobuf::uint8* target)
google::protobuf::uint8 *
    google::protobuf::internal::WireFormatLite::WriteInt64ToArray(
        int field_number,
        const google::protobuf::RepeatedField<google::protobuf::int64>& value,
        google::protobuf::uint8* target)
</code></pre>
@function wire_format_lite.int64_to_array(field_number, value, target)
@tparam integer field_number
@tparam integer/table value
@tparam uint8* target
@treturn uint8* return value of the wrapped function
@usage target = protobuf.wire_format_lite.int64_to_array(number, value, target)
*/
int tmc_protobuf_wire_format_lite_write_int64_to_array(lua_State *L)
    noexcept
{
    write_val_to_array(L, WireFormatLite::WriteInt64ToArray, int64);
    return 1;
}

/*
Wrapper of google::protobuf::internal::WireFormatLite::WriteUInt32ToArray.
<pre><code>
google::protobuf::uint8 *
    google::protobuf::internal::WireFormatLite::WriteUInt32ToArray(
        int field_number,
        google::protobuf::uint32 value,
        google::protobuf::uint8* target)
google::protobuf::uint8 *
    google::protobuf::internal::WireFormatLite::WriteUInt32ToArray(
        int field_number,
        const google::protobuf::RepeatedField<google::protobuf::uint32>& value,
        google::protobuf::uint8* target)
</code></pre>
@function wire_format_lite.uint32_to_array(field_number, value, target)
@tparam integer field_number
@tparam integer/table value
@tparam uint8* target
@treturn uint8* return value of the wrapped function
@usage target = protobuf.wire_format_lite.uint32_to_array(number, value, target)
*/
int tmc_protobuf_wire_format_lite_write_uint32_to_array(lua_State *L)
    noexcept
{
    write_val_to_array(L, WireFormatLite::WriteUInt32ToArray, uint32);
    return 1;
}

/*
Wrapper of google::protobuf::internal::WireFormatLite::WriteUInt64ToArray.
<pre><code>
google::protobuf::uint8 *
    google::protobuf::internal::WireFormatLite::WriteUInt64ToArray(
        int field_number,
        google::protobuf::uint64 value,
        google::protobuf::uint8* target)
google::protobuf::uint8 *
    google::protobuf::internal::WireFormatLite::WriteUInt64ToArray(
        int field_number,
        const google::protobuf::RepeatedField<google::protobuf::uint64>& value,
        google::protobuf::uint8* target)
</code></pre>
@function wire_format_lite.uint64_to_array(field_number, value, target)
@tparam integer field_number
@tparam integer/table value
@tparam uint8* target
@treturn uint8* return value of the wrapped function
@usage target = protobuf.wire_format_lite.uint64_to_array(number, value, target)
*/
int tmc_protobuf_wire_format_lite_write_uint64_to_array(lua_State *L)
    noexcept
{
    write_val_to_array(L, WireFormatLite::WriteUInt64ToArray, uint64);
    return 1;
}

/*
Wrapper of google::protobuf::internal::WireFormatLite::WriteSInt32ToArray.
<pre><code>
google::protobuf::uint8 *
    google::protobuf::internal::WireFormatLite::WriteSInt32ToArray(
        int field_number,
        google::protobuf::sint32 value,
        google::protobuf::uint8* target)
google::protobuf::uint8 *
    google::protobuf::internal::WireFormatLite::WriteSInt32ToArray(
        int field_number,
        const google::protobuf::RepeatedField<google::protobuf::sint32>& value,
        google::protobuf::uint8* target)
</code></pre>
@function wire_format_lite.sint32_to_array(field_number, value, target)
@tparam integer field_number
@tparam integer/table value
@tparam uint8* target
@treturn uint8* return value of the wrapped function
@usage target = protobuf.wire_format_lite.sint32_to_array(number, value, target)
*/
int tmc_protobuf_wire_format_lite_write_sint32_to_array(lua_State *L)
    noexcept
{
    write_val_to_array(L, WireFormatLite::WriteSInt32ToArray, int32);
    return 1;
}

/*
Wrapper of google::protobuf::internal::WireFormatLite::WriteSInt64ToArray.
<pre><code>
google::protobuf::uint8 *
    google::protobuf::internal::WireFormatLite::WriteSInt64ToArray(
        int field_number,
        google::protobuf::int64 value,
        google::protobuf::uint8* target)
google::protobuf::uint8 *
    google::protobuf::internal::WireFormatLite::WriteSInt64ToArray(
        int field_number,
        const google::protobuf::RepeatedField<google::protobuf::int64>& value,
        google::protobuf::uint8* target)
</code></pre>
@function wire_format_lite.sint64_to_array(field_number, value, target)
@tparam integer field_number
@tparam integer/table value
@tparam uint8* target
@treturn uint8* return value of the wrapped function
@usage target = protobuf.wire_format_lite.sint64_to_array(number, value, target)
*/
int tmc_protobuf_wire_format_lite_write_sint64_to_array(lua_State *L)
    noexcept
{
    write_val_to_array(L, WireFormatLite::WriteSInt64ToArray, int64);
    return 1;
}

/*
Wrapper of google::protobuf::internal::WireFormatLite::WriteFixed32ToArray.
<pre><code>
google::protobuf::uint8 *
    google::protobuf::internal::WireFormatLite::WriteFixed32ToArray(
        int field_number,
        google::protobuf::uint32 value,
        google::protobuf::uint8* target)
google::protobuf::uint8 *
    google::protobuf::internal::WireFormatLite::WriteFixed32ToArray(
        int field_number,
        const google::protobuf::RepeatedField<google::protobuf::uint32>& value,
        google::protobuf::uint8* target)
</code></pre>
@function wire_format_lite.fixed32_to_array(field_number, value, target)
@tparam integer field_number
@tparam integer/table value
@tparam uint8* target
@treturn uint8* return value of the wrapped function
@usage target = protobuf.wire_format_lite.fixed32_to_array(number, value, target)
*/
int tmc_protobuf_wire_format_lite_write_fixed32_to_array(lua_State *L)
    noexcept
{
    write_val_to_array(L, WireFormatLite::WriteFixed32ToArray, uint32);
    return 1;
}

/*
Wrapper of google::protobuf::internal::WireFormatLite::WriteFixed64ToArray.
<pre><code>
google::protobuf::uint8 *
    google::protobuf::internal::WireFormatLite::WriteFixed64ToArray(
        int field_number,
        google::protobuf::uint64 value,
        google::protobuf::uint8* target)
google::protobuf::uint8 *
    google::protobuf::internal::WireFormatLite::WriteFixed64ToArray(
        int field_number,
        const google::protobuf::RepeatedField<google::protobuf::uint64>& value,
        google::protobuf::uint8* target)
</code></pre>
@function wire_format_lite.fixed64_to_array(field_number, value, target)
@tparam integer field_number
@tparam integer/table value
@tparam uint8* target
@treturn uint8* return value of the wrapped function
@usage target = protobuf.wire_format_lite.fixed64_to_array(number, value, target)
*/
int tmc_protobuf_wire_format_lite_write_fixed64_to_array(lua_State *L)
    noexcept
{
    write_val_to_array(L, WireFormatLite::WriteFixed64ToArray, uint64);
    return 1;
}

/*
Wrapper of google::protobuf::internal::WireFormatLite::WriteSFixed32ToArray.
<pre><code>
google::protobuf::uint8 *
    google::protobuf::internal::WireFormatLite::WriteSFixed32ToArray(
        int field_number,
        google::protobuf::sint32 value,
        google::protobuf::uint8* target)
google::protobuf::uint8 *
    google::protobuf::internal::WireFormatLite::WriteSFixed32ToArray(
        int field_number,
        const google::protobuf::RepeatedField<google::protobuf::sint32>& value,
        google::protobuf::uint8* target)
</code></pre>
@function wire_format_lite.sfixed32_to_array(field_number, value, target)
@tparam integer field_number
@tparam integer/table value
@tparam uint8* target
@treturn uint8* return value of the wrapped function
@usage target = protobuf.wire_format_lite.sfixed32_to_array(number, value, target)
*/
int tmc_protobuf_wire_format_lite_write_sfixed32_to_array(lua_State *L)
    noexcept
{
    write_val_to_array(L, WireFormatLite::WriteSFixed32ToArray, int32);
    return 1;
}

/*
Wrapper of google::protobuf::internal::WireFormatLite::WriteSFixed64ToArray.
<pre><code>
google::protobuf::uint8 *
    google::protobuf::internal::WireFormatLite::WriteSFixed64ToArray(
        int field_number,
        google::protobuf::int64 value,
        google::protobuf::uint8* target)
google::protobuf::uint8 *
    google::protobuf::internal::WireFormatLite::WriteSFixed64ToArray(
        int field_number,
        const google::protobuf::RepeatedField<google::protobuf::int64>& value,
        google::protobuf::uint8* target)
</code></pre>
@function wire_format_lite.sfixed64_to_array(field_number, value, target)
@tparam integer field_number
@tparam integer/table value
@tparam uint8* target
@treturn uint8* return value of the wrapped function
@usage target = protobuf.wire_format_lite.sfixed64_to_array(number, value, target)
*/
int tmc_protobuf_wire_format_lite_write_sfixed64_to_array(lua_State *L)
    noexcept
{
    write_val_to_array(L, WireFormatLite::WriteSFixed64ToArray, int64);
    return 1;
}

/*
Wrapper of google::protobuf::internal::WireFormatLite::WriteFloatToArray.
<pre><code>
google::protobuf::uint8 *
    google::protobuf::internal::WireFormatLite::WriteFloatToArray(
        int field_number,
        float value,
        google::protobuf::uint8* target)
google::protobuf::uint8 *
    google::protobuf::internal::WireFormatLite::WriteFloatToArray(
        int field_number,
        const google::protobuf::RepeatedField<float>& value,
        google::protobuf::uint8* target)
</code></pre>
@function wire_format_lite.float_to_array(field_number, value, target)
@tparam integer field_number
@tparam number/table value
@tparam uint8* target
@treturn uint8* return value of the wrapped function
@usage target = protobuf.wire_format_lite.float_to_array(number, value, target)
*/
int tmc_protobuf_wire_format_lite_write_float_to_array(lua_State *L)
    noexcept
{
    write_val_to_array(L, WireFormatLite::WriteFloatToArray, float);
    return 1;
}

/*
Wrapper of google::protobuf::internal::WireFormatLite::WriteDoubleToArray.
<pre><code>
google::protobuf::uint8 *
    google::protobuf::internal::WireFormatLite::WriteDoubleToArray(
        int field_number,
        double value,
        google::protobuf::uint8* target)
google::protobuf::uint8 *
    google::protobuf::internal::WireFormatLite::WriteDoubleToArray(
        int field_number,
        const google::protobuf::RepeatedField<double>& value,
        google::protobuf::uint8* target)
</code></pre>
@function wire_format_lite.double_to_array(field_number, value, target)
@tparam integer field_number
@tparam number/table value
@tparam uint8* target
@treturn uint8* return value of the wrapped function
@usage target = protobuf.wire_format_lite.double_to_array(number, value, target)
*/
int tmc_protobuf_wire_format_lite_write_double_to_array(lua_State *L)
    noexcept
{
    write_val_to_array(L, WireFormatLite::WriteDoubleToArray, double);
    return 1;
}

/*
Wrapper of google::protobuf::internal::WireFormatLite::WriteBoolToArray.
<pre><code>
google::protobuf::uint8 *
    google::protobuf::internal::WireFormatLite::WriteBoolToArray(
        int field_number,
        bool value,
        google::protobuf::uint8* target)
google::protobuf::uint8 *
    google::protobuf::internal::WireFormatLite::WriteBoolToArray(
        int field_number,
        const google::protobuf::RepeatedField<bool>& value,
        google::protobuf::uint8* target)
</code></pre>
@function wire_format_lite.bool_to_array(field_number, value, target)
@tparam integer field_number
@tparam boolean/table value
@tparam uint8* target
@treturn uint8* return value of the wrapped function
@usage target = protobuf.wire_format_lite.bool_to_array(number, value, target)
*/
int tmc_protobuf_wire_format_lite_write_bool_to_array(lua_State *L)
    noexcept
{
    write_val_to_array(L, WireFormatLite::WriteBoolToArray, bool);
    return 1;
}

/*
Wrapper of google::protobuf::internal::WireFormatLite::WriteEnumToArray.
<pre><code>
google::protobuf::uint8 *
    google::protobuf::internal::WireFormatLite::WriteEnumToArray(
        int field_number,
        int value,
        google::protobuf::uint8* target)
google::protobuf::uint8 *
    google::protobuf::internal::WireFormatLite::WriteEnumToArray(
        int field_number,
        const google::protobuf::RepeatedField<int>& value,
        google::protobuf::uint8* target)
</code></pre>
@function wire_format_lite.enum_to_array(field_number, value, target)
@tparam integer field_number
@tparam integer/table value
@tparam uint8* target
@treturn uint8* return value of the wrapped function
@usage target = protobuf.wire_format_lite.enum_to_array(number, value, target)
*/
int tmc_protobuf_wire_format_lite_write_enum_to_array(lua_State *L)
    noexcept
{
    write_val_to_array(L, WireFormatLite::WriteEnumToArray, int);
    return 1;
}
