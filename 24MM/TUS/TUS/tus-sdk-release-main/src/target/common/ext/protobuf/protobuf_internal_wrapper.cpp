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
#include "protobuf_internal_wrapper.hpp"

#include "protobuf_helper.hpp"

#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/stubs/port.h>
#include <google/protobuf/parse_context.h>
#include <google/protobuf/stubs/stringpiece.h>
#include <google/protobuf/unknown_field_set.h>
#include <google/protobuf/metadata_lite.h>
#include <google/protobuf/wire_format.h>
#include <google/protobuf/repeated_field.h>

// shall be noexcept-safe
#define read_varint(L, READ_FUNC, TYPE)                                 \
    do                                                                  \
    {                                                                   \
        const char *p = tmc_protobuf_lua_check_char_ptr(L, 1);          \
        int type_num = tmc_protobuf_lua_check_int(L, 2);                \
        TYPE i = READ_FUNC(&p);                                         \
        tmc_protobuf_lua_push_char_ptr(L, p);                           \
        switch (type_num)                                               \
        {                                                               \
        case CPPTYPE_INT32:                                             \
        case CPPTYPE_ENUM:                                              \
            tmc_protobuf_lua_push_int32(L, i);                          \
            break;                                                      \
        case CPPTYPE_INT64:                                             \
            tmc_protobuf_lua_push_int64(L, i);                          \
            break;                                                      \
        case CPPTYPE_UINT32:                                            \
        case CPPTYPE_BOOL:                                              \
            tmc_protobuf_lua_push_uint32(L, i);                         \
            break;                                                      \
        case CPPTYPE_UINT64:                                            \
            tmc_protobuf_lua_push_uint64(L, i);                         \
            break;                                                      \
        default:                                                        \
            tmc_protobuf_lua_error(L, "Invalid type '%d'\n", type_num); \
            break;                                                      \
        }                                                               \
    } while (0)

// shall be noexcept-safe
#define packed_parser(L, PARSER_FUNC, TYPE)                             \
    do                                                                  \
    {                                                                   \
        tmc_protobuf_lua_check_table(L, 1);                             \
        const char *ptr = tmc_protobuf_lua_check_char_ptr(L, 2);        \
        ParseContext *ctx = tmc_protobuf_lua_check_parse_context(L, 3); \
        RepeatedField<TYPE> object;                                     \
        try {                                                           \
            ptr = PARSER_FUNC(&object, ptr, ctx);                       \
        } catch (...) { return tmc_protobuf_lua_error(L, __func__); }   \
        for (int i = 0; i < object.size(); i++)                         \
        {                                                               \
            tmc_protobuf_lua_push_##TYPE(L, object[i]);                 \
            lua_seti(L, 1, i + 1);                                      \
        }                                                               \
        tmc_protobuf_lua_push_char_ptr(L, ptr);                         \
    } while (0)

/*
 * fixme: google::protobuf::* may throw some exceptions.
 * for now, use try {} catch (...) {return tmc_protobuf_lua_error(L, __func__)}
 * to avoid triggering an abnormal termination.
 */
using google::protobuf::int32;
using google::protobuf::int64;
using google::protobuf::RepeatedField;
using google::protobuf::StringPiece;
using google::protobuf::uint32;
using google::protobuf::uint64;
using google::protobuf::UnknownFieldSet;
using google::protobuf::internal::CachedSize;
using google::protobuf::internal::DescriptorTable;
using google::protobuf::internal::InternalMetadata;
using google::protobuf::internal::ParseContext;

static bool ExpectTag(const char *ptr, uint32 tag)
{
    if (tag < 128)
    {
        return *ptr == (char)tag;
    }
    else
    {
        char buf[2] = {(char)(tag | 0x80), (char)(tag >> 7)};
        return std::memcmp(ptr, buf, 2) == 0;
    }
}

/*
Wrapper of google::protobuf::internal::ReadTag.
<pre><code>
const char *google::protobuf::internal::ReadTag(
    const char *p,
    google::protobuf::uint32 *out,
    google::protobuf::uint32 = 0U)
</code></pre>
Same as ParseVarint but only accept 5 bytes at most.
@function internal.read_tag(p)
@tparam char* p
@treturn[1] char* return value of the wrapped function
@treturn[2] integer tag
@usage ptr, tag = protobuf.internal.read_tag(ptr)
*/
int tmc_protobuf_internal_read_tag(lua_State *L)
    noexcept
{
    const char *p = tmc_protobuf_lua_check_char_ptr(L, 1);
    uint32 out;
    try {
        p = google::protobuf::internal::ReadTag(p, &out);
    } catch (...) { return tmc_protobuf_lua_error(L, __func__); }
    tmc_protobuf_lua_push_char_ptr(L, p);
    tmc_protobuf_lua_push_uint32(L, out);
    return 2;
}

/*
Wrapper of google::protobuf::internal::InlineGreedyStringParser.
<pre><code>
const char *google::protobuf::internal::InlineGreedyStringParser(
    std::string *s,
    const char *ptr,
    google::protobuf::internal::ParseContext *ctx)
</code></pre>
All the string parsers with or without UTF checking and for all CTypes.
@function internal.inline_greedy_string_parser(ptr, ctx)
@tparam char* ptr
@tparam ParseContext ctx
@treturn[1] char* return value of the wrapped function
@treturn[2] string parsed string
@usage ptr, tag = protobuf.internal.inline_greedy_string_parser(ptr, ctx)
*/
int tmc_protobuf_internal_inline_greedy_string_parser(lua_State *L)
    noexcept
{
    const char *ptr = tmc_protobuf_lua_check_char_ptr(L, 1);
    ParseContext *ctx = tmc_protobuf_lua_check_parse_context(L, 2);
    std::string str;
    try {
        ptr = google::protobuf::internal::InlineGreedyStringParser(&str, ptr, ctx);
    } catch (...) { return tmc_protobuf_lua_error(L, __func__); }
    tmc_protobuf_lua_push_char_ptr(L, ptr);
    lua_pushlstring(L, str.c_str(), str.size());
    return 2;
}

/*
Wrapper of google::protobuf::internal::VerifyUTF8.
<pre><code>
bool google::protobuf::internal::VerifyUTF8(
    google::protobuf::StringPiece s,
    const char *field_name)
</code></pre>
Verification of utf8
@function internal.verify_utf8(s, ptr)
@tparam string s
@tparam char* ptr
@treturn boolean return value of the wrapped function
@usage ret = protobuf.internal.verify_utf8(str, protobuf.char.nullptr)
*/
int tmc_protobuf_internal_verify_utf8(lua_State *L)
    noexcept
{
    size_t len = 0;
    const char *s = luaL_checklstring(L, 1, &len);
    const char *field_name = tmc_protobuf_lua_check_char_ptr(L, 2);
    StringPiece sp(s, len);
    bool b;
    try {
        b = google::protobuf::internal::VerifyUTF8(sp, field_name);
    } catch (...) { return tmc_protobuf_lua_error(L, __func__); }
    tmc_protobuf_lua_push_bool(L, b);
    return 1;
}

/*
Wrapper of google::protobuf::internal::ReadVarint64.
<pre><code>
google::protobuf::uint64 google::protobuf::internal::ReadVarint64(const char **p)
</code></pre>
@function internal.read_varint64(ptr, type_num)
@tparam char* ptr
@tparam integer type_num
@treturn[1] char* ptr
@treturn[2] integer return value of the wrapped function
@usage ptr, self.id = protobuf.internal.read_varint64(ptr, protobuf.internal.INT32)
*/
int tmc_protobuf_internal_read_varint64(lua_State *L)
    noexcept
{
    read_varint(L, google::protobuf::internal::ReadVarint64, uint64);
    return 2;
}

/*
Wrapper of google::protobuf::internal::ReadVarint32.
<pre><code>
google::protobuf::uint32 google::protobuf::internal::ReadVarint32(const char **p)
</code></pre>
@function read_varint32(ptr, type_num)
@tparam char* ptr
@tparam integer type_num
@treturn[1] char* ptr
@treturn[2] integer return value of the wrapped function
@usage ptr, self.id = protobuf.internal.read_varint32(ptr, protobuf.internal.UINT32)
*/
int tmc_protobuf_internal_read_varint32(lua_State *L)
    noexcept
{
    read_varint(L, google::protobuf::internal::ReadVarint32, uint32);
    return 2;
}

/*
Wrapper of google::protobuf::internal::ReadVarintZigZag64.
<pre><code>
google::protobuf::int64 google::protobuf::internal::ReadVarintZigZag64(const char **p)
</code></pre>
@function internal.read_varint_zigzag64(ptr, type_num)
@tparam char* ptr
@tparam integer type_num
@treturn[1] char* ptr
@treturn[2] integer return value of the wrapped function
@usage ptr, self.id = protobuf.internal.read_varint_zigzag64(ptr, protobuf.internal.INT64)
*/
int tmc_protobuf_internal_read_varint_zigzag64(lua_State *L)
    noexcept
{
    read_varint(L, google::protobuf::internal::ReadVarintZigZag64, int64);
    return 2;
}

/*
Wrapper of google::protobuf::internal::ReadVarintZigZag32.
<pre><code>
google::protobuf::int32 google::protobuf::internal::ReadVarintZigZag32(const char **p)
</code></pre>
@function internal.read_varint_zigzag64(ptr, type_num)
@tparam char* ptr
@tparam integer type_num
@treturn[1] char* ptr
@treturn[2] integer return value of the wrapped function
@usage ptr, self.id = protobuf.internal.read_varint_zigzag64(ptr, protobuf.internal.INT64)
*/
int tmc_protobuf_internal_read_varint_zigzag32(lua_State *L)
    noexcept
{
    read_varint(L, google::protobuf::internal::ReadVarintZigZag32, int32);
    return 2;
}

/*
Wrapper of google::protobuf::internal::ExpectTag.
<pre><code>
template <google::protobuf::uint32 tag>
    bool google::protobuf::internal::ReadVarintZigZag32(const char *ptr)
</code></pre>
@function internal.expect_tag(ptr, tag)
@tparam char* ptr
@tparam integer tag
@treturn boolean return value of the wrapped function
@usage ret = protobuf.internal.expect_tag(ptr, 114)
*/
int tmc_protobuf_internal_expect_tag(lua_State *L)
    noexcept
{
    const char *ptr = tmc_protobuf_lua_check_char_ptr(L, 1);
    uint32 tag = tmc_protobuf_lua_check_uint32(L, 2);
    bool b = ExpectTag(ptr, tag);
    tmc_protobuf_lua_push_bool(L, b);
    return 1;
}

/*
Wrapper of google::protobuf::internal::UnknownFieldParse.
<pre><code>
const char *google::protobuf::internal::UnknownFieldParse(
    google::protobuf::uint32 tag,
    std::string *unknown,
    const char *ptr,
    google::protobuf::internal::ParseContext *ctx)
</code></pre>
This is a helper to for the UnknownGroupLiteParse but is actually also useful in the generated code.
It uses overload on std::string* vs UnknownFieldSet* to make the generated code isomorphic between full and lite.
@function internal.unknown_field_parse(tag, unknown, ptr, ctx)
@tparam integer tag
@tparam string unknown
@tparam char* ptr
@tparam ParseContext ctx
@treturn[1] char* return value of the wrapped function
@treturn[2] string parsed string of unknown fields
@usage ptr, unknown = protobuf.internal.unknown_field_parse(tag, self:get_internal_metadata():unknown_fields(), ptr, ctx)
*/
int tmc_protobuf_internal_unknown_field_parse(lua_State *L)
    noexcept
{
    uint64 tag = tmc_protobuf_lua_check_uint64(L, 1);
    size_t l = 0;
    const char *str = luaL_checklstring(L, 2, &l);
    std::string unknown(str, l);
    const char *ptr = tmc_protobuf_lua_check_char_ptr(L, 3);
    ParseContext *ctx = tmc_protobuf_lua_check_parse_context(L, 4);
    try {
        ptr = google::protobuf::internal::UnknownFieldParse(tag, &unknown, ptr, ctx);
    } catch (...) { return tmc_protobuf_lua_error(L, __func__); }
    tmc_protobuf_lua_push_char_ptr(L, ptr);
    lua_pushlstring(L, unknown.c_str(), unknown.length());
    return 2;
}

/*
Wrapper of google::protobuf::internal::UnalignedLoad.
<pre><code>
template<class T> T google::protobuf::internal::UnalignedLoad(const char *p)
</code></pre>
@function internal.unaligned_load(ptr, type_num)
@tparam char* ptr
@tparam integer type_num
@treturn integer/number return value of the wrapped function
@usage self.point = protobuf.internal.unaligned_load(ptr, protobuf.internal.DOUBLE)
*/
int tmc_protobuf_internal_unaligned_load(lua_State *L)
    noexcept
{
    char *p = tmc_protobuf_lua_check_char_ptr(L, 1);
    int type_num = tmc_protobuf_lua_check_int(L, 2);
    switch (type_num)
    {

#define CASE(TYPE_CONST, TYPE)                                         \
    case TYPE_CONST:                                                   \
    {                                                                  \
        TYPE val;                                                      \
        try {                                                          \
            val = google::protobuf::internal::UnalignedLoad<TYPE>(p);  \
        } catch (...) { return tmc_protobuf_lua_error(L, __func__); }  \
        tmc_protobuf_lua_push_##TYPE(L, val);                          \
        break;                                                         \
    }

        CASE(CPPTYPE_INT32, int32)
        CASE(CPPTYPE_INT64, int64)
        CASE(CPPTYPE_UINT32, uint32)
        CASE(CPPTYPE_UINT64, uint64)
        CASE(CPPTYPE_DOUBLE, double)
        CASE(CPPTYPE_FLOAT, float)

#undef CASE

    default:
        return tmc_protobuf_lua_error(L, "Invalid type '%d'\n", type_num);
    }
    return 1;
}

/*
Wrapper of google::protobuf::internal::PackedInt32Parser.
<pre><code>
const char *google::protobuf::internal::PackedInt32Parser(
    void *object,
    const char *ptr,
    google::protobuf::internal::ParseContext *ctx)
</code></pre>
The packed parsers parse repeated numeric primitives directly into the corresponding field.
@function internal.packed_int32_parser(object, ptr, ctx)
@tparam table object
@tparam char* ptr
@tparam ParseContext ctx
@treturn char* return value of the wrapped function
@usage ptr = protobuf.internal.packed_int32_parser(value, ptr, ctx)
*/
int tmc_protobuf_internal_packed_int32_parser(lua_State *L)
    noexcept
{
    packed_parser(L, google::protobuf::internal::PackedInt32Parser, int32);
    return 1;
}

/*
Wrapper of google::protobuf::internal::PackedUInt32Parser.
<pre><code>
const char *google::protobuf::internal::PackedUInt32Parser(
    void *object,
    const char *ptr,
    google::protobuf::internal::ParseContext *ctx)
</code></pre>
@function internal.packed_uint32_parser(object, ptr, ctx)
@tparam table object
@tparam char* ptr
@tparam ParseContext ctx
@treturn char* return value of the wrapped function
@usage ptr = protobuf.internal.packed_uint32_parser(value, ptr, ctx)
*/
int tmc_protobuf_internal_packed_uint32_parser(lua_State *L)
    noexcept
{
    packed_parser(L, google::protobuf::internal::PackedUInt32Parser, uint32);
    return 1;
}

/*
Wrapper of google::protobuf::internal::PackedInt64Parser.
<pre><code>
const char *google::protobuf::internal::PackedInt64Parser(
    void *object,
    const char *ptr,
    google::protobuf::internal::ParseContext *ctx)
</code></pre>
@function internal.packed_int64_parser(object, ptr, ctx)
@tparam table object
@tparam char* ptr
@tparam ParseContext ctx
@treturn char* return value of the wrapped function
@usage ptr = protobuf.internal.packed_int64_parser(value, ptr, ctx)
*/
int tmc_protobuf_internal_packed_int64_parser(lua_State *L)
    noexcept
{
    packed_parser(L, google::protobuf::internal::PackedInt64Parser, int64);
    return 1;
}

/*
Wrapper of google::protobuf::internal::PackedUInt64Parser.
<pre><code>
const char *google::protobuf::internal::PackedUInt64Parser(
    void *object,
    const char *ptr,
    google::protobuf::internal::ParseContext *ctx)
</code></pre>
@function internal.packed_uint64_parser(object, ptr, ctx)
@tparam table object
@tparam char* ptr
@tparam ParseContext ctx
@treturn char* return value of the wrapped function
@usage ptr = protobuf.internal.packed_uint64_parser(value, ptr, ctx)
*/
int tmc_protobuf_internal_packed_uint64_parser(lua_State *L)
    noexcept
{
    packed_parser(L, google::protobuf::internal::PackedUInt64Parser, uint64);
    return 1;
}

/*
Wrapper of google::protobuf::internal::PackedSInt32Parser.
<pre><code>
const char *google::protobuf::internal::PackedSInt32Parser(
    void *object,
    const char *ptr,
    google::protobuf::internal::ParseContext *ctx)
</code></pre>
@function internal.packed_sint32_parser(object, ptr, ctx)
@tparam table object
@tparam char* ptr
@tparam ParseContext ctx
@treturn char* return value of the wrapped function
@usage ptr = protobuf.internal.packed_sint32_parser(value, ptr, ctx)
*/
int tmc_protobuf_internal_packed_sint32_parser(lua_State *L)
    noexcept
{
    packed_parser(L, google::protobuf::internal::PackedSInt32Parser, int32);
    return 1;
}

/*
Wrapper of google::protobuf::internal::PackedSInt64Parser.
<pre><code>
const char *google::protobuf::internal::PackedSInt64Parser(
    void *object,
    const char *ptr,
    google::protobuf::internal::ParseContext *ctx)
</code></pre>
@function internal.packed_sint64_parser(object, ptr, ctx)
@tparam table object
@tparam char* ptr
@tparam ParseContext ctx
@treturn char* return value of the wrapped function
@usage ptr = protobuf.internal.packed_sint64_parser(value, ptr, ctx)
*/
int tmc_protobuf_internal_packed_sint64_parser(lua_State *L)
    noexcept
{
    packed_parser(L, google::protobuf::internal::PackedSInt64Parser, int64);
    return 1;
}

/*
Wrapper of google::protobuf::internal::PackedEnumParser.
<pre><code>
const char *google::protobuf::internal::PackedEnum64Parser(
    void *object,
    const char *ptr,
    google::protobuf::internal::ParseContext *ctx)
</code></pre>
@function internal.packed_enum_parser(object, ptr, ctx)
@tparam table object
@tparam char* ptr
@tparam ParseContext ctx
@treturn char* return value of the wrapped function
@usage ptr = protobuf.internal.packed_enum_parser(value, ptr, ctx)
*/
int tmc_protobuf_internal_packed_enum_parser(lua_State *L)
    noexcept
{
    packed_parser(L, google::protobuf::internal::PackedEnumParser, int32);
    return 1;
}

/*
Wrapper of google::protobuf::internal::PackedBoolParser.
<pre><code>
const char *google::protobuf::internal::PackedBool64Parser(
    void *object,
    const char *ptr,
    google::protobuf::internal::ParseContext *ctx)
</code></pre>
@function internal.packed_bool_parser(object, ptr, ctx)
@tparam table object
@tparam char* ptr
@tparam ParseContext ctx
@treturn char* return value of the wrapped function
@usage ptr = protobuf.internal.packed_bool_parser(value, ptr, ctx)
*/
int tmc_protobuf_internal_packed_bool_parser(lua_State *L)
    noexcept
{
    packed_parser(L, google::protobuf::internal::PackedBoolParser, bool);
    return 1;
}

/*
Wrapper of google::protobuf::internal::PackedFixed32Parser.
<pre><code>
const char *google::protobuf::internal::PackedFixed32Parser(
    void *object,
    const char *ptr,
    google::protobuf::internal::ParseContext *ctx)
</code></pre>
@function internal.packed_fixed32_parser(object, ptr, ctx)
@tparam table object
@tparam char* ptr
@tparam ParseContext ctx
@treturn char* return value of the wrapped function
@usage ptr = protobuf.internal.packed_fixed32_parser(value, ptr, ctx)
*/
int tmc_protobuf_internal_packed_fixed32_parser(lua_State *L)
    noexcept
{
    packed_parser(L, google::protobuf::internal::PackedFixed32Parser, uint32);
    return 1;
}

/*
Wrapper of google::protobuf::internal::PackedSFixed32Parser.
<pre><code>
const char *google::protobuf::internal::PackedSFixed32Parser(
    void *object,
    const char *ptr,
    google::protobuf::internal::ParseContext *ctx)
</code></pre>
@function internal.packed_sfixed32_parser(object, ptr, ctx)
@tparam table object
@tparam char* ptr
@tparam ParseContext ctx
@treturn char* return value of the wrapped function
@usage ptr = protobuf.internal.packed_sfixed32_parser(value, ptr, ctx)
*/
int tmc_protobuf_internal_packed_sfixed32_parser(lua_State *L)
    noexcept
{
    packed_parser(L, google::protobuf::internal::PackedSFixed32Parser, int32);
    return 1;
}

/*
Wrapper of google::protobuf::internal::PackedFixed64Parser.
<pre><code>
const char *google::protobuf::internal::PackedFixed64Parser(
    void *object,
    const char *ptr,
    google::protobuf::internal::ParseContext *ctx)
</code></pre>
@function internal.packed_fixed64_parser(object, ptr, ctx)
@tparam table object
@tparam char* ptr
@tparam ParseContext ctx
@treturn char* return value of the wrapped function
@usage ptr = protobuf.internal.packed_fixed64_parser(value, ptr, ctx)
*/
int tmc_protobuf_internal_packed_fixed64_parser(lua_State *L)
    noexcept
{
    packed_parser(L, google::protobuf::internal::PackedFixed64Parser, uint64);
    return 1;
}

/*
Wrapper of google::protobuf::internal::PackedSFixed64Parser.
<pre><code>
const char *google::protobuf::internal::PackedSFixed64Parser(
    void *object,
    const char *ptr,
    google::protobuf::internal::ParseContext *ctx)
</code></pre>
@function internal.packed_sfixed64_parser(object, ptr, ctx)
@tparam table object
@tparam char* ptr
@tparam ParseContext ctx
@treturn char* return value of the wrapped function
@usage ptr = protobuf.internal.packed_sfixed64_parser(value, ptr, ctx)
*/
int tmc_protobuf_internal_packed_sfixed64_parser(lua_State *L)
    noexcept
{
    packed_parser(L, google::protobuf::internal::PackedSFixed64Parser, int64);
    return 1;
}

/*
Wrapper of google::protobuf::internal::PackedFloatParser.
<pre><code>
const char *google::protobuf::internal::PackedFloatParser(
    void *object,
    const char *ptr,
    google::protobuf::internal::ParseContext *ctx)
</code></pre>
@function internal.packed_float_parser(object, ptr, ctx)
@tparam table object
@tparam char* ptr
@tparam ParseContext ctx
@treturn char* return value of the wrapped function
@usage ptr = protobuf.internal.packed_float_parser(value, ptr, ctx)
*/
int tmc_protobuf_internal_packed_float_parser(lua_State *L)
    noexcept
{
    packed_parser(L, google::protobuf::internal::PackedFloatParser, float);
    return 1;
}

/*
Wrapper of google::protobuf::internal::PackedDoubleParser.
<pre><code>
const char *google::protobuf::internal::PackedDoubleParser(
    void *object,
    const char *ptr,
    google::protobuf::internal::ParseContext *ctx)
</code></pre>
@function internal.packed_double_parser(object, ptr, ctx)
@tparam table object
@tparam char* ptr
@tparam ParseContext ctx
@treturn char* return value of the wrapped function
@usage ptr = protobuf.internal.packed_double_parser(self.point, ptr, ctx)
*/
int tmc_protobuf_internal_packed_double_parser(lua_State *L)
    noexcept
{
    packed_parser(L, google::protobuf::internal::PackedDoubleParser, double);
    return 1;
}
