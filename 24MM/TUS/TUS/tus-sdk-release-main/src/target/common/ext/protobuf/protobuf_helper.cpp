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
#include "protobuf_helper.hpp"

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <string>

static int tmc_protobuf_lua_type_error(lua_State *L, int arg, const char *tname, const char *fmt, ...);
static void *tmc_protobuf_lua_check_userdata(lua_State *L, int arg, const char *tname);

void tmc_protobuf_lua_push_udata(lua_State *L, const void *udata, const char *metacontext)
{
    const void **udata_ = (const void **)lua_newuserdata(L, sizeof(void *));
    *udata_ = udata;
    luaL_setmetatable(L, metacontext);
}

void *tmc_protobuf_lua_check_udata(lua_State *L, int arg, const char *tname)
{
    void **udata = (void **)tmc_protobuf_lua_check_userdata(L, arg, tname);
    return *udata;
}

void *tmc_protobuf_lua_check_udata_ptr_ptr(lua_State *L, int arg, const char *tname)
{
    void **udata = (void **)tmc_protobuf_lua_check_userdata(L, arg, tname);
    return udata;
}

void tmc_protobuf_lua_push_message_lite(lua_State *L, const MessageLiteLua *message_lite)
{
    lua_newtable(L);
    luaL_setmetatable(L, PROTOBUF_MESSAGE_LITE_META_CONTEXT);
    const MessageLiteLua **udata = (const MessageLiteLua **)lua_newuserdata(L, sizeof(void *));
    *udata = message_lite;
    luaL_setmetatable(L, PROTOBUF_MESSAGE_LITE_META_CONTEXT);
    lua_setfield(L, -2, PROTOBUF_DATA_CONTEXT);
}

MessageLiteLua *tmc_protobuf_lua_check_message_lite(lua_State *L, int arg)
{
    MessageLiteLua **message_lite;
    tmc_protobuf_lua_check_table(L, arg);
    lua_getfield(L, arg, PROTOBUF_DATA_CONTEXT);
    message_lite = (MessageLiteLua **)tmc_protobuf_lua_check_userdata(L, -1, PROTOBUF_MESSAGE_LITE_META_CONTEXT);
    lua_pop(L, 1);
    (*message_lite)->SetLuaState(L);
    (*message_lite)->SetLuaSelfIndex(arg);
    return *message_lite;
}

MessageLiteLua **tmc_protobuf_lua_check_message_lite_ptr_ptr(lua_State *L, int arg)
{
    MessageLiteLua **message_lite;
    tmc_protobuf_lua_check_table(L, arg);
    lua_getfield(L, arg, PROTOBUF_DATA_CONTEXT);
    message_lite = (MessageLiteLua **)tmc_protobuf_lua_check_userdata(L, -1, PROTOBUF_MESSAGE_LITE_META_CONTEXT);
    lua_pop(L, 1);
    return message_lite;
}

int tmc_protobuf_lua_check_int(lua_State *L, int arg)
{
    long long num = tmc_protobuf_lua_check_integer(L, arg);
    if (num < INT_MIN || num > INT_MAX)
    {
        tmc_protobuf_lua_error(L, "A value is out of range. Expected a int32_t value, buf caught: %lld.\n", num);
        return 0;
    }
    else
    {
        return (int)num;
    }
}

size_t tmc_protobuf_lua_check_size_t(lua_State *L, int arg)
{
    long long num = tmc_protobuf_lua_check_integer(L, arg);
    if (num < 0 || (unsigned long long)num > SIZE_MAX)
    {
        tmc_protobuf_lua_error(L, "A value is out of range. Expected a unsigned long value, buf caught: %lld.\n", num);
        return 0;
    }
    else
    {
        return (size_t)num;
    }
}

int32_t tmc_protobuf_lua_check_int32(lua_State *L, int arg)
{
    long long num = tmc_protobuf_lua_check_integer(L, arg);
    if (num < INT32_MIN || num > INT32_MAX)
    {
        tmc_protobuf_lua_error(L, "A value is out of range. Expected a int32_t value, buf caught: %lld.\n", num);
        return 0;
    }
    else
    {
        return (int32_t)num;
    }
}

uint32_t tmc_protobuf_lua_check_uint32(lua_State *L, int arg)
{
    long long num = tmc_protobuf_lua_check_integer(L, arg);
    if (num < 0 || num > UINT32_MAX)
    {
        tmc_protobuf_lua_error(L, "A value is out of range. Expected a uint32_t value, buf caught: %lld.\n", num);
        return 0;
    }
    else
    {
        return (uint32_t)num;
    }
}

int64_t tmc_protobuf_lua_check_int64(lua_State *L, int arg)
{
    long long num = tmc_protobuf_lua_check_integer(L, arg);
    if (num < INT64_MIN || num > INT64_MAX)
    {
        tmc_protobuf_lua_error(L, "A value is out of range. Expected a int64_t value, buf caught: %lld.\n", num);
        return 0;
    }
    else
    {
        return (int64_t)num;
    }
}

uint64_t tmc_protobuf_lua_check_uint64(lua_State *L, int arg)
{
    const char *str = luaL_checkstring(L, arg);
    char *end_ptr;
    errno = 0;
    long long num = strtoll(str, &end_ptr, 10);
    if (end_ptr == str)
    {
        tmc_protobuf_lua_error(L, "Cannot parse string to int64_t. Expected a uint64_t value, buf caught: %s.\n", str);
        return 0;
    }
    else if (*end_ptr != '\0')
    {
        tmc_protobuf_lua_error(L, "Extra characters on string. Expected a uint64_t value, but caught: %s.\n", end_ptr);
        return 0;
    }
    else if ((num == LLONG_MIN && errno == ERANGE) || (num < 0))
    {
        tmc_protobuf_lua_error(L, "A value is out of range (minus). Expected a uint64_t value, buf caught: %s.\n", str);
        return 0;
    }
    else if (num == LLONG_MAX && errno == ERANGE)
    {
        errno = 0;
        unsigned long long unum = strtoull(str, &end_ptr, 10);
        if ((unum == ULLONG_MAX && errno == ERANGE) || (unum > UINT64_MAX))
        {
            tmc_protobuf_lua_error(L, "A value is out of range (plus). Expected a uint64_t value, buf caught: %s.\n", str);
            return 0;
        }
        else
        {
            return (uint64_t)unum;
        }
    }
    else
    {
        return (uint64_t)num;
    }
}

double tmc_protobuf_lua_check_double(lua_State *L, int arg)
{
    const char *str = luaL_checkstring(L, arg);
    char *end_ptr;
    errno = 0;
    double num = strtod(str, &end_ptr);
    if (end_ptr == str)
    {
        tmc_protobuf_lua_error(L, "Cannot parse string to double. Expected a double value, buf caught: %s.\n", str);
        return 0;
    }
    else if (*end_ptr != '\0')
    {
        tmc_protobuf_lua_error(L, "Extra characters on string. Expected a double value, but caught: %s.\n", end_ptr);
        return 0;
    }
    else if ((num == -HUGE_VAL || num == HUGE_VAL) && errno == ERANGE)
    {
        tmc_protobuf_lua_error(L, "A value is out of range. Expected a double value, buf caught: %s.\n", str);
        return 0;
    }
    else
    {
        return num;
    }
}

float tmc_protobuf_lua_check_float(lua_State *L, int arg)
{
    const char *str = luaL_checkstring(L, arg);
    char *end_ptr;
    errno = 0;
    float num = strtof(str, &end_ptr);
    if (end_ptr == str)
    {
        tmc_protobuf_lua_error(L, "Cannot parse string to float. Expected a float value, buf caught: %s.\n", str);
        return 0;
    }
    else if (*end_ptr != '\0')
    {
        tmc_protobuf_lua_error(L, "Extra characters on string. Expected a number value, but caught: %s.\n", end_ptr);
        return 0;
    }
    else if ((num == -HUGE_VALF || num == HUGE_VALF) && errno == ERANGE)
    {
        tmc_protobuf_lua_error(L, "A value is out of range. Expected a float value, buf caught: %s.\n", str);
        return 0;
    }
    else
    {
        return num;
    }
}

bool tmc_protobuf_lua_check_bool(lua_State *L, int arg)
{
    if (lua_isboolean(L, arg))
    {
        int rtn = lua_toboolean(L, arg);
        return rtn ? true : false;
    }
    else
    {
        tmc_protobuf_lua_error(L, "Checking boolean failed. Expected boolean, buf caught: %s.\n", luaL_typename(L, arg));
        return false;
    }
}

int tmc_protobuf_lua_error(lua_State *L, const char *fmt, ...)
{
    lua_Debug ar;
    if (lua_getstack(L, 0, &ar) && lua_getinfo(L, "Sln", &ar))
    {
        fprintf(stderr, "ERROR: %s:%d ([%s] %s) ", ar.short_src, ar.currentline, ar.namewhat, ar.name ? ar.name : "(?)");
    }
    else
    {
        fprintf(stderr, "ERROR: ???:0 ");
    }
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    return lua_error(L);
}

static int tmc_protobuf_lua_type_error(lua_State *L, int arg, const char *tname, const char *fmt, ...)
{
    lua_Debug ar;
    if (lua_getstack(L, 0, &ar) && lua_getinfo(L, "Sln", &ar))
    {
        fprintf(stderr, "ERROR: %s:%d ([%s] %s) ", ar.short_src, ar.currentline, ar.namewhat, ar.name ? ar.name : "(?)");
    }
    else
    {
        fprintf(stderr, "ERROR: ???:0 ");
    }
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    luaL_checkudata(L, arg, tname);
    return lua_error(L);
}

long long tmc_protobuf_lua_check_integer(lua_State *L, int arg)
{
    const char *str = luaL_checkstring(L, arg);
    char *end_ptr;
    errno = 0;
    long long num = strtoll(str, &end_ptr, 10);
    if (end_ptr == str)
    {
        tmc_protobuf_lua_error(L, "Cannot parse string to integer. Expected a integer buf caught: %s.\n", str);
        return 0;
    }
    else if (*end_ptr != '\0')
    {
        tmc_protobuf_lua_error(L, "Extra characters on string. Expected a integer, but caught: %s.\n", end_ptr);
        return 0;
    }
    else if ((num == LLONG_MIN || num == LLONG_MAX) && errno == ERANGE)
    {
        tmc_protobuf_lua_error(L, "A value is out of range. Expected a integer, buf caught: %s.\n", str);
        return 0;
    }
    else
    {
        return num;
    }
}

void tmc_protobuf_lua_check_table(lua_State *L, int arg)
{
    if (!lua_istable(L, arg))
    {
        tmc_protobuf_lua_error(L, "Invalid type. Expected table, but caught: %s\n", luaL_typename(L, arg));
    }
}

static void *tmc_protobuf_lua_check_userdata(lua_State *L, int arg, const char *tname)
{
    void *userdata = luaL_testudata(L, arg, tname);
    if (userdata == nullptr)
    {
        tmc_protobuf_lua_type_error(L, arg, tname, "Invalid type. Expected %s, but caught a different type.\n", tname);
        return nullptr;
    }
    else
    {
        return userdata;
    }
}
