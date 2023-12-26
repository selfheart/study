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
#include "protobuf_message_lite.hpp"

#include "protobuf_helper.hpp"

namespace pb = google::protobuf;

static int traceback(lua_State *L)
{
    const char *message = lua_tostring(L, 1);
    if (message)
    {
        luaL_traceback(L, L, message, 1);
    }
    else if (!lua_isnoneornil(L, 1))
    {
        if (!luaL_callmeta(L, 1, "__tostring"))
        {
            lua_pushliteral(L, "(no error message)");
        }
    }
    else
    {
        lua_pushliteral(L, "(nil)");
    }
    return 1;
}

MessageLiteLua::~MessageLiteLua()
{
    _internal_metadata_.Delete<std::string>();
}

MessageLiteLua *MessageLiteLua::New() const
{
    return CreateMaybeMessage<MessageLiteLua>(nullptr);
}

MessageLiteLua *MessageLiteLua::New(pb::Arena *arena) const
{
    return CreateMaybeMessage<MessageLiteLua>(arena);
}

void MessageLiteLua::CheckTypeAndMergeFrom(const MessageLite &other)
{
    const MessageLiteLua *from = (const MessageLiteLua *)&other;
    _internal_metadata_.MergeFrom<std::string>(from->_internal_metadata_);
    L = from->L;
    index = from->index;
}

int MessageLiteLua::GetCachedSize() const
{
    return cached_size.Get();
}

void MessageLiteLua::SetCachedSize(int size)
{
    cached_size.Set(size);
}

bool MessageLiteLua::IsInitialized() const
{
    return true;
}

std::string MessageLiteLua::GetTypeName() const
{
    return type_name;
}

void MessageLiteLua::SetTypeName(const char *type_name)
{
    this->type_name.assign(type_name);
}

void MessageLiteLua::Clear()
{
    lua_getfield(L, index, "clear");
    lua_pushvalue(L, index);
    int nargs = 1;
    int error_func = lua_gettop(L) - nargs;
    lua_pushcfunction(L, traceback);
    lua_insert(L, error_func);
    int error_code = lua_pcall(L, nargs, 0, error_func);
    lua_remove(L, error_func);
    if (error_code != LUA_OK)
    {
        CallError(error_code, "A call to clear() function failed.");
    }
}

size_t MessageLiteLua::ByteSizeLong() const
{
    lua_getfield(L, index, "byte_size_long");
    lua_pushvalue(L, index);
    int nargs = 1;
    int error_func = lua_gettop(L) - nargs;
    lua_pushcfunction(L, traceback);
    lua_insert(L, error_func);
    int error_code = lua_pcall(L, nargs, 1, error_func);
    lua_remove(L, error_func);
    if (error_code != LUA_OK)
    {
        CallError(error_code, "A call to byte_size_long() function failed.");
        return 0;
    }
    size_t rtn = tmc_protobuf_lua_check_size_t(L, -1);
    return rtn;
}

const char *MessageLiteLua::_InternalParse(const char *ptr, pb::internal::ParseContext *ctx)
{
    lua_getfield(L, index, "internal_parse");
    lua_pushvalue(L, index);
    tmc_protobuf_lua_push_char_ptr(L, ptr);
    tmc_protobuf_lua_push_parse_context(L, ctx);
    int nargs = 3;
    int error_func = lua_gettop(L) - nargs;
    lua_pushcfunction(L, traceback);
    lua_insert(L, error_func);
    int error_code = lua_pcall(L, nargs, 1, error_func);
    lua_remove(L, error_func);
    if (error_code != LUA_OK)
    {
        CallError(error_code, "A call to internal_parse() function failed.");
        return nullptr;
    }
    return tmc_protobuf_lua_check_char_ptr(L, -1);
}

pb::uint8 *MessageLiteLua::_InternalSerialize(pb::uint8 *ptr, pb::io::EpsCopyOutputStream *stream) const
{
    lua_getfield(L, index, "internal_serialize");
    lua_pushvalue(L, index);
    tmc_protobuf_lua_push_uint8_ptr(L, ptr);
    tmc_protobuf_lua_push_eps_copy_output_stream(L, stream);
    int nargs = 3;
    int error_func = lua_gettop(L) - nargs;
    lua_pushcfunction(L, traceback);
    lua_insert(L, error_func);
    int error_code = lua_pcall(L, nargs, 1, error_func);
    lua_remove(L, error_func);
    if (error_code != LUA_OK)
    {
        CallError(error_code, "A call to internal_serialize() function failed.");
        return nullptr;
    }
    return tmc_protobuf_lua_check_uint8_ptr(L, -1);
}

const pb::internal::InternalMetadata *MessageLiteLua::GetInternalMetadata() const
{
    return &_internal_metadata_;
}

void MessageLiteLua::SetLuaState(lua_State *L)
{
    this->L = L;
}

void MessageLiteLua::SetLuaSelfIndex(int index)
{
    this->index = index;
}

int MessageLiteLua::CallError(int error_code, const char *msg) const
{
    static const char *code_msg[] = {
        "?",                     // LUA_OK
        "?",                     // LUA_YIELD
        "run error",             // LUA_ERRRUN
        "syntax error",          // LUA_ERRSYNTAX
        "memory error",          // LUA_ERRMEM
        "GC error",              // LUA_ERRGCMM
        "message handler error", // LUA_ERRERR
    };
    const char *err = luaL_checkstring(L, -1);
    return tmc_protobuf_lua_error(L, "CALL ERROR: (%s) %s ERROR HANDLER: %s\n", code_msg[error_code], msg, err);
}
