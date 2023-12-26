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
#include "protobuf_internal_metadata_wrapper.hpp"

#include "protobuf_helper.hpp"

#include <string>
#include <google/protobuf/metadata_lite.h>

/*
 * fixme: google::protobuf::* may throw some exceptions.
 * for now, use try {} catch (...) {return tmc_protobuf_lua_error(L, __func__)}
 * to avoid triggering an abnormal termination.
 */
using google::protobuf::internal::GetEmptyString;
using google::protobuf::internal::InternalMetadata;

/*
Wrapper of google::protobuf::internal::InternalMetadata::Clear.
<pre><code>
template <typename T> void google::protobuf::internal::InternalMetadata::Clear()
</code></pre>
@function internal_metadata.clear()
@usage self:get_internal_metadata():clear
*/
int tmc_protobuf_internal_metadata_clear(lua_State *L)
    noexcept
{
    InternalMetadata *self = tmc_protobuf_lua_check_internal_metadata(L, 1);
    try {
        self->Clear<std::string>();
    } catch (...) { return tmc_protobuf_lua_error(L, __func__); }
    return 0;
}

/*
Wrapper of google::protobuf::internal::InternalMetadata::have_unknown_fields.
<pre><code>
bool google::protobuf::internal::InternalMetadata::have_unknown_fields() const
</code></pre>
@function internal_metadata.have_unknown_fields()
@treturn boolean return value of the wrapped function
@usage self:get_internal_metadata():have_unknown_fields()
*/
int tmc_protobuf_internal_metadata_have_unknown_fields(lua_State *L)
    noexcept
{
    InternalMetadata *self = tmc_protobuf_lua_check_internal_metadata(L, 1);
    bool b;
    try {
        b = self->have_unknown_fields();
    } catch (...) { return tmc_protobuf_lua_error(L, __func__); }
    tmc_protobuf_lua_push_bool(L, b);
    return 1;
}

/*
Wrapper of google::protobuf::internal::InternalMetadata::unknown_fields.
<br>
<pre><code>
template <typename T> const std::string &
    google::protobuf::internal::InternalMetadata::unknown_fields(
        const std::string &(*default_instance)()) const
</code></pre>
@function internal_metadata.unknown_fields()
@treturn string return value of the wrapped function
@usage self:get_internal_metadata():unknown_fields()
*/
int tmc_protobuf_internal_metadata_unknown_fields(lua_State *L)
    noexcept
{
    InternalMetadata *self = tmc_protobuf_lua_check_internal_metadata(L, 1);
    try {
        const std::string str = self->unknown_fields<std::string>(GetEmptyString);
        lua_pushlstring(L, str.c_str(), str.length());
    } catch (...) { return tmc_protobuf_lua_error(L, __func__); }
    return 1;
}

/*
Set unknown fields to internal metadata.
@function internal_metadata.set_unknown_fields(unknown)
@tparam string unknown
@usage self:get_internal_metadata():set_unknown_fields(unknown)
*/
int tmc_protobuf_internal_metadata_set_unknown_fields(lua_State *L)
    noexcept
{
    InternalMetadata *self = tmc_protobuf_lua_check_internal_metadata(L, 1);
    size_t l = 0;
    const char *unknown = luaL_checklstring(L, 2, &l);
    try {
        std::string *str = self->mutable_unknown_fields<std::string>();
        str->assign(unknown, l);
    } catch (...) { return tmc_protobuf_lua_error(L, __func__); }
    return 0;
}
