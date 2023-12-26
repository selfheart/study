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
#include "protobuf_message_lite_wrapper.hpp"

#include "protobuf_message_lite.hpp"
#include "protobuf_helper.hpp"

#include <string>
#include <google/protobuf/metadata_lite.h>

/*
 * fixme: google::protobuf::* may throw some exceptions.
 * for now, use try {} catch (...) {return tmc_protobuf_lua_error(L, __func__)}
 * to avoid triggering an abnormal termination.
 */
using google::protobuf::uint8;
using google::protobuf::internal::InternalMetadata;
using google::protobuf::internal::ParseContext;
using google::protobuf::io::EpsCopyOutputStream;

/*
Wrapper of MessageLiteLua::MessageLiteLua.
<pre><code>
MessageLiteLua::MessageLiteLua()
</code></pre>
Constructor of MessageLiteLua which wraps google::protobuf::MessageLite class.
The method creates message_lite instance.
@function message_lite.new()
@treturn table message_lite instance
@usage message = protobuf.message_lite.new()
*/
int tmc_protobuf_message_lite_new(lua_State *L)
    noexcept
{
    MessageLiteLua *message;
    try {
        message  = new (std::nothrow) MessageLiteLua();
    } catch (...) { return tmc_protobuf_lua_error(L, __func__); }
    if (message == nullptr)
    {
        return tmc_protobuf_lua_error(L, "message_lite.new() failed.\n");
    }
    const char *type_name = luaL_checkstring(L, 1);
    try {
        message->SetTypeName(type_name);
    } catch (...) { return tmc_protobuf_lua_error(L, __func__); }
    tmc_protobuf_lua_push_message_lite(L, message);
    return 1;
}

/*
Wrapper of MessageLiteLua::~MessageLiteLua.
<pre><code>
MessageLiteLua::~MessageLiteLua()
</code></pre>
Destructor of MessageLiteLua.
The method deletes message_lite instance.
Though garbage collection deletes the instance automatically, a user dose not use the method.
@function message_lite.delete()
@usage message:delete()
*/
int tmc_protobuf_message_lite_delete(lua_State *L)
    noexcept
{
    MessageLiteLua **self = tmc_protobuf_lua_check_message_lite_ptr_ptr(L, 1);
    delete *self;
    *self = nullptr;
    return 0;
}

/*
Wrapper of MessageLiteLua::GetCachedSize.
<pre><code>
int MessageLiteLua::GetCachedSize() const
</code></pre>
@function message_lite.get_cached_size()
@treturn integer return value of the wrapped function
@usage size = message:get_cached_size()
*/
int tmc_protobuf_message_lite_get_cached_size(lua_State *L)
    noexcept
{
    MessageLiteLua *self = tmc_protobuf_lua_check_message_lite(L, 1);
    int cached_size;
    try {
        cached_size = self->GetCachedSize();
    } catch (...) { return tmc_protobuf_lua_error(L, __func__); }
    tmc_protobuf_lua_push_int(L, cached_size);
    return 1;
}

/*
Wrapper of MessageLiteLua::SetCachedSize.
<pre><code>
void MessageLiteLua::SetCachedSize(int size)
</code></pre>
@function message_lite.set_cached_size(size)
@tparam integer size
@usage message:set_cached_size(size)
*/
int tmc_protobuf_message_lite_set_cached_size(lua_State *L)
    noexcept
{
    MessageLiteLua *self = tmc_protobuf_lua_check_message_lite(L, 1);
    int size = tmc_protobuf_lua_check_int(L, 2);
    try {
        self->SetCachedSize(size);
    } catch (...) { return tmc_protobuf_lua_error(L, __func__); }
    return 0;
}

/*
Wrapper of MessageLiteLua::GetInternalMetadata.
<pre><code>
const google::protobuf::internal::InternalMetadata *
    MessageLiteLua::GetInternalMetadata() const
</code></pre>
@function message_lite.get_internal_metadata()
@treturn InternalMetadata return value of the wrapped function
@usage internal_metadata = message:get_internal_metadata()
*/
int tmc_protobuf_message_lite_get_internal_metadata(lua_State *L)
    noexcept
{
    MessageLiteLua *self = tmc_protobuf_lua_check_message_lite(L, 1);
    tmc_protobuf_lua_push_internal_metadata(L, self->GetInternalMetadata());
    return 1;
}

/*
Wrapper of MessageLiteLua::SerializeToString.
<pre><code>
bool google::protobuf::MessageLite::SerializeToString(std::string *output) const
</code></pre>
Serialize the message and return it.
If the result fails, return nil.
@function message_lite.serialize_to_string()
@treturn string encoding binary
@usage bytes = message:serialize_to_string()
*/
int tmc_protobuf_message_lite_serialize_to_string(lua_State *L)
    noexcept
{
    MessageLiteLua *self = tmc_protobuf_lua_check_message_lite(L, 1);
    std::string str;
    bool success;
    try {
        success = self->SerializeToString(&str);
    } catch (...) { return tmc_protobuf_lua_error(L, __func__); }
    if (success)
    {
        lua_pushlstring(L, str.c_str(), str.size());
    }
    else
    {
        lua_pushnil(L);
    }
    return 1;
}

/*
Wrapper of MessageLiteLua::ParseFromString.
<pre><code>
bool google::protobuf::MessageLite::ParseFromString(const std::string &data)
</code></pre>
Parses a protocol buffer contained in a string.
Returns true on success.
This function takes a string in the (non-human-readable) binary wire format, matching the encoding output by protobuf.message_lite.serialize_to_string().
@function message_lite.parse_from_string(bytes)
@tparam string bytes encoding binary
@treturn boolean success or not
@usage success = message:parse_from_string(bytes)
*/
int tmc_protobuf_message_lite_parse_from_string(lua_State *L)
    noexcept
{
    MessageLiteLua *self = tmc_protobuf_lua_check_message_lite(L, 1);
    size_t len;
    const char *data = luaL_checklstring(L, 2, &len);
    std::string str(data, len);
    bool b;
    try {
        b = self->ParseFromString(str);
    } catch (...) { return tmc_protobuf_lua_error(L, __func__); }
    tmc_protobuf_lua_push_bool(L, b);
    return 1;
}
