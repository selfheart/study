/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */

/* clang-format off */
/***
@module tup_parser
@copyright (C) 2022 TOYOTA MOTOR CORPORATION, All Rights Reserved.
@usage
local tup_parser = require("tup_parser")
local parser, err = tup_parser.from_file("/path/to/file")
err = parser:validate()
local tlvs, metadata
tlvs, err = parser:get_tlv({id=tup_parser.TLV_ID.UPDATEFLOW})
metadata, err = tlvs[1]:get_value()
*/
/* clang-format on */

/* system header */
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdlib.h>
#include <string.h>

/* user header */
#include "tup_parser.h"

/* object macro */
#define TUP_PARSER_META_CONTEXT "tup_parser.context"
#define TUP_TLV_META_CONTEXT "tlv.context"
#define TUP_TLV_LENGTH_MAX 0xFFFFFFFFFFFFL

/* function macro */
#define SET_CONSTANT(v, n)           \
    do {                             \
        lua_pushinteger(L, v);       \
        lua_setfield(L, -2, &#v[n]); \
    } while (0)

/* typedef definition */

/* enum definition */

/* struct/union definition */
typedef struct context {
    lua_State *L;
    tup_parser *parser;
} context;

typedef struct tlv_context {
    tup_parser *parser;
    tup_tlv *tlv;
} tlv_context;

/* static variable */

/* static function */
static context *
context_check(lua_State *L, int index)
{
    return (context *)luaL_testudata(L, index, TUP_PARSER_META_CONTEXT);
}

static tlv_context *
tlv_context_check(lua_State *L, int index)
{
    return (tlv_context *)luaL_testudata(L, index, TUP_TLV_META_CONTEXT);
}

/* clang-format off */
/***
Creates TUP parser object which parses TUP from file.
@function from_file
@tparam string path
@treturn userdata instance
@treturn userdata error object
@usage
local tup_parser = require("tup_parser")
local parser, err = tup_parser.from_file("/path/to/file")
*/
/* clang-format on */
static int
from_file(lua_State *L)
{
    if (0 == lua_isstring(L, 1)) {
        lua_pushnil(L);
        lua_pushinteger(L, TUP_PARSER_ERROR_INVALID_ARGUMENT);
        return 2;
    }
    const char *filepath = luaL_checkstring(L, 1);

    context *ctx = (context *)lua_newuserdata(L, sizeof(context));
    ctx->L = L;
    ctx->parser = NULL;

    tup_parser *parser = NULL;
    int ret = tup_create_parser(&parser);
    if (0 != ret) {
        lua_pushnil(L);
        lua_pushinteger(L, ret);
        return 2;
    }

    tup_file_stream file_stream = {};
    file_stream.path = filepath;

    tup_stream_param param = {};
    param.type = TUP_STREAM_FILE;
    param.stream.file = &file_stream;

    ret = tup_attach_stream(parser, &param);
    if (0 != ret) {
        (void)tup_destroy_parser(&parser);
        lua_pushnil(L);
        lua_pushinteger(L, ret);
        return 2;
    }

    ctx->parser = parser;
    luaL_getmetatable(L, TUP_PARSER_META_CONTEXT);
    lua_setmetatable(L, -2);

    return 1;
}

/* clang-format off */
/***
Creates TUP parser object which parses TUP from url.
@function from_url
@tparam string url
@treturn userdata instance
@treturn userdata error object
@usage
local tup_parser = require("tup_parser")
local parser, err = tup_parser.from_url("localhost/path/to/file")
*/
/* clang-format on */
static int
from_url(lua_State *L)
{
    if (0 == lua_isstring(L, 1)) {
        lua_pushnil(L);
        lua_pushinteger(L, TUP_PARSER_ERROR_INVALID_ARGUMENT);
        return 2;
    }
    const char *url = luaL_checkstring(L, 1);

    context *ctx = (context *)lua_newuserdata(L, sizeof(context));
    ctx->L = L;
    ctx->parser = NULL;

    tup_parser *parser = NULL;
    int ret = tup_create_parser(&parser);
    if (0 != ret) {
        lua_pushnil(L);
        lua_pushinteger(L, ret);
        return 2;
    }

    tup_net_stream net_stream = {};
    net_stream.url = url;

    tup_stream_param param = {};
    param.type = TUP_STREAM_NET;
    param.stream.net = &net_stream;

    ret = tup_attach_stream(parser, &param);
    if (0 != ret) {
        (void)tup_destroy_parser(&parser);
        lua_pushnil(L);
        lua_pushinteger(L, ret);
        return 2;
    }

    ctx->parser = parser;
    luaL_getmetatable(L, TUP_PARSER_META_CONTEXT);
    lua_setmetatable(L, -2);

    return 1;
}

/* clang-format off */
/***
Creates TUP parser object which parses TUP from TUP parser.
@function from_parser
@tparam userdata parser
@tparam integer index (should be within 1 and <i>get_inner_package_count()</i>)
@treturn userdata instance
@treturn userdata error object
@usage
local tup_parser = require("tup_parser")
local outer_parser, err = tup_parser.from_url("localhost/path/to/file")
local inner_parser
inner_parser, err = tup_parser.from_parser(outer_parser, 1)
*/
/* clang-format on */
static int
from_parser(lua_State *L)
{
    if (0 == lua_isuserdata(L, 1)) {
        lua_pushnil(L);
        lua_pushinteger(L, TUP_PARSER_ERROR_INVALID_ARGUMENT);
        return 1;
    }
    context *ctx = context_check(L, 1);
    if (NULL == ctx) {
        lua_pushnil(L);
        lua_pushinteger(L, TUP_PARSER_ERROR_INVALID_ARGUMENT);
        return 1;
    }

    if (0 == lua_isinteger(L, 2)) {
        lua_pushnil(L);
        lua_pushinteger(L, TUP_PARSER_ERROR_INVALID_ARGUMENT);
        return 2;
    }
    lua_Integer index = luaL_checkinteger(L, 2) - 1;
    if (index < 0 || UINT32_MAX < index) {
        lua_pushnil(L);
        lua_pushinteger(L, TUP_PARSER_ERROR_INVALID_ARGUMENT);
        return 2;
    }

    context *new_ctx = (context *)lua_newuserdata(L, sizeof(context));
    new_ctx->L = L;
    new_ctx->parser = NULL;

    tup_parser *parser = NULL;
    int ret = tup_create_parser(&parser);
    if (0 != ret) {
        lua_pushnil(L);
        lua_pushinteger(L, ret);
        return 2;
    }

    struct tup_loop_stream stream = { .parent = ctx->parser, .index = index };
    tup_stream_param param = { .type = TUP_STREAM_LOOP,
        .stream = { .loop = &stream } };
    ret = tup_attach_stream(parser, &param);
    if (0 != ret) {
        (void)tup_destroy_parser(&parser);
        lua_pushnil(L);
        lua_pushinteger(L, ret);
        return 2;
    }

    new_ctx->parser = parser;
    luaL_getmetatable(L, TUP_PARSER_META_CONTEXT);
    lua_setmetatable(L, -2);

    return 1;
}

static int
cleanup(lua_State *L)
{
    context *ctx = context_check(L, 1);
    if (NULL == ctx || NULL == ctx->parser) {
        return 0;
    }

    tup_parser *parser = ctx->parser;
    ctx->parser = NULL;

    (void)tup_detach_stream(parser);
    (void)tup_destroy_parser(&parser);

    return 0;
}

/* clang-format off */
/***
ID of TLV object with TLV type defined as field.
@table TLV_ID
@field INDEX Type and position of the TLV objects
@field POS Positional information of the inner packages
@field ICV_TREE Information of the ICV tree structure
@field ICV_ARRAY Information of the tampering verification targets
@field IPKGINFO Inner package information
@field IPKGNAME Inner package name
@field COMP_ALL Has inner package with entire region being compressed
@field COMP_FIXED Has inner package compressed with a fixed length block size
@field COMP_VAR Has inner package compressed with a variable length block size
@field CRYPT Has encrypted inner package
@field UPDATEFLOW Update flow information
@field VERSION Inner package version
@field DOMAIN Domain where the inner package will be applied to
@field ORDER Processing order of compression, encryption, and ICV generation
@field DELTA_PATCH Delta update patch
@field ICV_AFTER_DATA Information on ICV of patched data
@field ICV_BLOCK Information of the ICV block
@usage
local tup_parser = require("tup_parser")
local index_tlv_id = tup_parser.TLV_ID.INDEX
local updateflow_tlv_id = tup_parser.TLV_ID.UPDATEFLOW
*/
/***
Flag to set actions on getting the inner package.
@table FLAG
@field DECRYPT Decrypt inner package
@field DECOMPRESS Decompress inner package
@field ICV_VERIFY Verify integrity of inner package by ICV
@usage
local tup_parser = require("tup_parser")
local flag = tup_parser.FLAG.DECRYPT | tup_parser.FLAG.ICV_VERIFY
*/
/***
Error values used in tup_parser.
@table ERROR
@field INVALID_ARGUMENT invalid argument was given
@field OUT_OF_RANGE out of range value
@field CANNOT_ALLOCATE_MEMORY can not allocate memory
@field INTEGRITY_CHECK failed to integrity check
@field NOT_FOUND_TLV not found TLV
@field INVALID_ALGORITHM invalid algorithm
@field INVALID_KEY_INFORMATION invalid key information
@field NOT_FOUND_ICV_INFORMATION not found ICV information
@field CANNOT_OPEN_STREAM can not open stream
@field CANNOT_READ_STREAM can not read stream
@field CANNOT_CLOSE_STREAM can not close stream
@field INVALID_ENDIAN invalid endian
@field INVALID_META_CONTEXT invalid meta context
@field MISMATCH_TLV_ID mismatch TLV ID
@field SMALL_DATA_SIZE small data size
@field IMPORT_KEY failed to import key
@field VERIFY_SIGNATURE failed to verify signature
@field GENERATE_ICV failed to generate ICV
*/
/* clang-format on */
static void
luaopen_tup_parser_wrapper_lib(lua_State *L)
{
    static const struct luaL_Reg tup_parser_lib[] = {
        { "from_file", from_file }, { "from_url", from_url },
        { "from_parser", from_parser }, { NULL, NULL }
    };
    luaL_newlib(L, tup_parser_lib);

    lua_pushstring(L, "TLV_ID");
    lua_newtable(L);
    size_t len = strlen("TUP_ID_");
    SET_CONSTANT(TUP_ID_INVALID, len);
    SET_CONSTANT(TUP_ID_INDEX, len);
    SET_CONSTANT(TUP_ID_POS, len);
    SET_CONSTANT(TUP_ID_ICV_TREE, len);
    SET_CONSTANT(TUP_ID_ICV_ARRAY, len);
    SET_CONSTANT(TUP_ID_IPKGINFO, len);
    SET_CONSTANT(TUP_ID_IPKGNAME, len);
    SET_CONSTANT(TUP_ID_COMP_ALL, len);
    SET_CONSTANT(TUP_ID_COMP_FIXED, len);
    SET_CONSTANT(TUP_ID_COMP_VAR, len);
    SET_CONSTANT(TUP_ID_CRYPT, len);
    SET_CONSTANT(TUP_ID_UPDATEFLOW, len);
    SET_CONSTANT(TUP_ID_VERSION, len);
    SET_CONSTANT(TUP_ID_DOMAIN, len);
    SET_CONSTANT(TUP_ID_ORDER, len);
    SET_CONSTANT(TUP_ID_DELTA_PATCH, len);
    SET_CONSTANT(TUP_ID_ICV_AFTER_DATA, len);
    SET_CONSTANT(TUP_ID_ICV_BLOCK, len);
    lua_settable(L, -3);

    lua_pushstring(L, "FLAG");
    lua_newtable(L);
    len = strlen("TUP_FLAG_");
    SET_CONSTANT(TUP_FLAG_DECRYPT, len);
    SET_CONSTANT(TUP_FLAG_DECOMPRESS, len);
    SET_CONSTANT(TUP_FLAG_ICV_VERIFY, len);
    lua_settable(L, -3);

    lua_pushstring(L, "ERROR");
    lua_newtable(L);
    len = strlen("TUP_PARSER_ERROR_");
    SET_CONSTANT(TUP_PARSER_ERROR_INVALID_ARGUMENT, len);
    SET_CONSTANT(TUP_PARSER_ERROR_OUT_OF_RANGE, len);
    SET_CONSTANT(TUP_PARSER_ERROR_CANNOT_ALLOCATE_MEMORY, len);
    SET_CONSTANT(TUP_PARSER_ERROR_INTEGRITY_CHECK, len);
    SET_CONSTANT(TUP_PARSER_ERROR_NOT_FOUND_TLV, len);
    SET_CONSTANT(TUP_PARSER_ERROR_INVALID_ALGORITHM, len);
    SET_CONSTANT(TUP_PARSER_ERROR_INVALID_KEY_INFORMATION, len);
    SET_CONSTANT(TUP_PARSER_ERROR_NOT_FOUND_ICV_INFORMATION, len);
    SET_CONSTANT(TUP_PARSER_ERROR_CANNOT_OPEN_STREAM, len);
    SET_CONSTANT(TUP_PARSER_ERROR_CANNOT_READ_STREAM, len);
    SET_CONSTANT(TUP_PARSER_ERROR_CANNOT_CLOSE_STREAM, len);
    SET_CONSTANT(TUP_PARSER_ERROR_INVALID_ENDIAN, len);
    SET_CONSTANT(TUP_PARSER_ERROR_INVALID_META_CONTEXT, len);
    SET_CONSTANT(TUP_PARSER_ERROR_MISMATCH_TLV_ID, len);
    SET_CONSTANT(TUP_PARSER_ERROR_SMALL_DATA_SIZE, len);
    SET_CONSTANT(TUP_PARSER_ERROR_DECOMPRESS, len);
    SET_CONSTANT(TUP_PARSER_ERROR_IMPORT_KEY, len);
    SET_CONSTANT(TUP_PARSER_ERROR_VERIFY_SIGNATURE, len);
    SET_CONSTANT(TUP_PARSER_ERROR_GENERATE_ICV, len);
    lua_settable(L, -3);
}

/* clang-format off */
/***
Validates the integrity of TUP.
@function validate
@treturn userdata error object (Succeeded: nil, Failed: not nil)
@usage
local tup_parser = require("tup_parser")
local parser, err = tup_parser.from_file("/path/to/file")
err = parser:validate()
*/
/* clang-format on */
static int
validate(lua_State *L)
{
    context *ctx = context_check(L, 1);
    if (NULL == ctx) {
        lua_pushinteger(L, TUP_PARSER_ERROR_INVALID_META_CONTEXT);
        return 1;
    }

    int ret = tup_validate(ctx->parser);
    if (0 != ret) {
        lua_pushinteger(L, ret);
        return 1;
    }

    return 0;
}

/* clang-format off */
/***
Gets the TLV format object of the given TLV ID.
@function get_tlv
@tparam table args
  <p> - <span class="parameter">id         (mandatory)</span> (<b><i>integer</i></b>)</p>
  <p> - <span class="parameter">parent_tlv (optional)</span>  (<b><i>userdata</i></b>)</p>
@treturn userdata array of TLV format object
@treturn userdata error object
@usage
local tup_parser = require("tup_parser")
local parser, err = tup_parser.from_file("/path/to/file")
local tlvs
tlvs, err = parser:get_tlv({id=tup_parser.TLV_ID.UPDATEFLOW})
*/
/* clang-format on */
static int
get_tlv(lua_State *L)
{
    context *ctx = context_check(L, 1);
    if (NULL == ctx) {
        lua_pushnil(L);
        lua_pushinteger(L, TUP_PARSER_ERROR_INVALID_META_CONTEXT);
        return 2;
    }

    if (0 == lua_istable(L, 2)) {
        lua_pushnil(L);
        lua_pushinteger(L, TUP_PARSER_ERROR_INVALID_ARGUMENT);
        return 2;
    }

    int type = lua_getfield(L, -1, "id");
    if (LUA_TNUMBER != type) {
        lua_pushnil(L);
        lua_pushinteger(L, TUP_PARSER_ERROR_INVALID_ARGUMENT);
        return 2;
    }
    if (0 == lua_isinteger(L, -1)) {
        lua_pushnil(L);
        lua_pushinteger(L, TUP_PARSER_ERROR_INVALID_ARGUMENT);
        return 2;
    }
    lua_Integer id = luaL_checkinteger(L, -1);
    if (id < 0 || UINT16_MAX < id) {
        lua_pushnil(L);
        lua_pushinteger(L, TUP_PARSER_ERROR_OUT_OF_RANGE);
        return 2;
    }

    type = lua_getfield(L, -2, "parent_tlv");
    tup_get_tlv_param param;
    if (LUA_TNIL == type) {
        param.parent_tlv = NULL;
    } else if (LUA_TUSERDATA == type) {
        tlv_context *tlv_ctx = tlv_context_check(L, -1);
        if (NULL == tlv_ctx) {
            lua_pushnil(L);
            lua_pushinteger(L, TUP_PARSER_ERROR_INVALID_META_CONTEXT);
            return 2;
        }
        param.parent_tlv = tlv_ctx->tlv;
    } else {
        lua_pushnil(L);
        lua_pushinteger(L, TUP_PARSER_ERROR_INVALID_ARGUMENT);
        return 2;
    }

    param.tlv = NULL;
    int ret = tup_find_tlv(ctx->parser, id, &param);
    if (0 != ret) {
        lua_pushnil(L);
        lua_pushinteger(L, ret);
        return 2;
    }

    if (NULL == param.tlv) {
        lua_pushnil(L);
        lua_pushinteger(L, TUP_PARSER_ERROR_NOT_FOUND_TLV);
        return 2;
    }

    lua_newtable(L);
    while (NULL != param.tlv) {
        tlv_context *tlv_ctx = (tlv_context *)lua_newuserdata(
          L, sizeof(tlv_context));
        tlv_ctx->parser = ctx->parser;
        tlv_ctx->tlv = param.tlv;
        luaL_getmetatable(L, TUP_TLV_META_CONTEXT);
        lua_setmetatable(L, -2);
        lua_rawseti(L, -2, 1);

        int ret = tup_find_tlv(ctx->parser, id, &param);
        if (0 != ret) {
            lua_pushnil(L);
            lua_pushinteger(L, ret);
            return 2;
        }
    }

    return 1;
}

/* clang-format off */
/***
Gets the number of inner package in TUP.
@function get_inner_package_count
@treturn integer package count
@treturn userdata error object
@usage
local tup_parser = require("tup_parser")
local parser, err = tup_parser.from_file("/path/to/file")
local count
count, err = parser:get_inner_package_count()
*/
/* clang-format on */
static int
get_inner_package_count(lua_State *L)
{
    context *ctx = context_check(L, 1);
    if (NULL == ctx) {
        lua_pushnil(L);
        lua_pushinteger(L, TUP_PARSER_ERROR_INVALID_META_CONTEXT);
        return 2;
    }

    uint32_t count;
    int ret = tup_get_inner_package_count(ctx->parser, &count);
    if (0 != ret) {
        lua_pushnil(L);
        lua_pushinteger(L, ret);
        return 2;
    }
    lua_pushinteger(L, count);

    return 1;
}

/* clang-format off */
/***
Gets TLV format object of IPKGINFO of the given inner package index.
@function get_inner_package_info
@tparam integer index (should be within 1 and <i>get_inner_package_count()</i>)
@treturn userdata TLV format object
@treturn userdata error object
@usage
local tup_parser = require("tup_parser")
local parser, err = tup_parser.from_file("/path/to/file")
local info
info, err = parser:get_inner_package_info(1)
*/
/* clang-format on */
static int
get_inner_package_info(lua_State *L)
{
    context *ctx = context_check(L, 1);
    if (NULL == ctx) {
        lua_pushnil(L);
        lua_pushinteger(L, TUP_PARSER_ERROR_INVALID_META_CONTEXT);
        return 2;
    }

    if (0 == lua_isinteger(L, 2)) {
        lua_pushnil(L);
        lua_pushinteger(L, TUP_PARSER_ERROR_INVALID_ARGUMENT);
        return 2;
    }
    lua_Integer index = luaL_checkinteger(L, 2) - 1;
    if (index < 0 || UINT32_MAX < index) {
        lua_pushnil(L);
        lua_pushinteger(L, TUP_PARSER_ERROR_OUT_OF_RANGE);
        return 2;
    }

    tup_tlv *ptlv = NULL;
    int ret = tup_get_inner_package_info(ctx->parser, index, &ptlv);
    if (0 != ret) {
        lua_pushnil(L);
        lua_pushinteger(L, ret);
        return 2;
    }

    tlv_context *tlv_ctx = (tlv_context *)lua_newuserdata(
      L, sizeof(tlv_context));
    tlv_ctx->parser = ctx->parser;
    tlv_ctx->tlv = ptlv;
    luaL_getmetatable(L, TUP_TLV_META_CONTEXT);
    lua_setmetatable(L, -2);

    return 1;
}

/* clang-format off */
/***
Gets inner package data of the given inner package index.
@function get_inner_package_data
@tparam table args
  <p> - <span class="parameter">index  (mandatory)</span> (<b><i>integer</i></b>)</p> (should be within 1 and <i>get_inner_package_count()</i>)
  <p> - <span class="parameter">flags  (optional)</span>  (<b><i>integer</i></b>)</p>
  <p> - <span class="parameter">offset (optional)</span>  (<b><i>integer</i></b>)</p>
  <p> - <span class="parameter">size   (optional)</span>  (<b><i>integer</i></b>)</p>
@treturn string inner package data
@treturn userdata error object
@usage
local tup_parser = require("tup_parser")
local parser, err = tup_parser.from_file("/path/to/file")
local data
data, err = parser:get_inner_package_data({index=1, flags=tup_parser.FLAG.ICV_VERIFY})
*/
/* clang-format on */
static int
get_inner_package_data(lua_State *L)
{
    context *ctx = context_check(L, 1);
    if (NULL == ctx) {
        lua_pushnil(L);
        lua_pushinteger(L, TUP_PARSER_ERROR_INVALID_META_CONTEXT);
        return 2;
    }

    if (0 == lua_istable(L, 2)) {
        lua_pushnil(L);
        lua_pushinteger(L, TUP_PARSER_ERROR_INVALID_ARGUMENT);
        return 2;
    }

    int type = lua_getfield(L, -1, "index");
    if (LUA_TNUMBER != type) {
        lua_pushnil(L);
        lua_pushinteger(L, TUP_PARSER_ERROR_INVALID_ARGUMENT);
        return 2;
    }
    if (0 == lua_isinteger(L, -1)) {
        lua_pushnil(L);
        lua_pushinteger(L, TUP_PARSER_ERROR_INVALID_ARGUMENT);
        return 2;
    }
    lua_Integer tmp_index = luaL_checkinteger(L, -1) - 1;
    if (tmp_index < 0 || UINT32_MAX < tmp_index) {
        lua_pushnil(L);
        lua_pushinteger(L, TUP_PARSER_ERROR_OUT_OF_RANGE);
        return 2;
    }
    uint32_t index = (uint32_t)tmp_index;

    uint32_t flags = 0;
    type = lua_getfield(L, -2, "flags");
    if (LUA_TNIL != type) {
        if (LUA_TNUMBER != type) {
            lua_pushnil(L);
            lua_pushinteger(L, TUP_PARSER_ERROR_INVALID_ARGUMENT);
            return 2;
        }
        if (0 == lua_isinteger(L, -1)) {
            lua_pushnil(L);
            lua_pushinteger(L, TUP_PARSER_ERROR_INVALID_ARGUMENT);
            return 2;
        }
        lua_Integer tmp_flags = luaL_checkinteger(L, -1);
        if (tmp_flags < 0 || UINT32_MAX < tmp_flags) {
            lua_pushnil(L);
            lua_pushinteger(L, TUP_PARSER_ERROR_OUT_OF_RANGE);
            return 2;
        }
        if (tmp_flags & ~TUP_FLAG_ALL) {
            lua_pushnil(L);
            lua_pushinteger(L, TUP_PARSER_ERROR_INVALID_ARGUMENT);
            return 2;
        }
        flags = (uint32_t)tmp_flags;
    }

    tup_get_range range;
    int ret = tup_get_inner_package_size(ctx->parser, index, &range);
    if (0 != ret) {
        lua_pushnil(L);
        lua_pushinteger(L, ret);
        return 2;
    }

    uint64_t offset = 0;
    type = lua_getfield(L, -3, "offset");
    if (LUA_TNIL != type) {
        if (LUA_TNUMBER != type) {
            lua_pushnil(L);
            lua_pushinteger(L, TUP_PARSER_ERROR_INVALID_ARGUMENT);
            return 2;
        }
        if (0 == lua_isinteger(L, -1)) {
            lua_pushnil(L);
            lua_pushinteger(L, TUP_PARSER_ERROR_INVALID_ARGUMENT);
            return 2;
        }
        lua_Integer tmp_offset = luaL_checkinteger(L, -1);
        if (tmp_offset < 0) {
            lua_pushnil(L);
            lua_pushinteger(L, TUP_PARSER_ERROR_OUT_OF_RANGE);
            return 2;
        }
        offset = (uint64_t)tmp_offset;
    }

    if (range.raw_size <= offset) {
        lua_pushnil(L);
        lua_pushinteger(L, TUP_PARSER_ERROR_OUT_OF_RANGE);
        return 2;
    }

    uint64_t size = range.raw_size - offset;
    type = lua_getfield(L, -4, "size");
    if (LUA_TNIL != type) {
        if (LUA_TNUMBER != type) {
            lua_pushnil(L);
            lua_pushinteger(L, TUP_PARSER_ERROR_INVALID_ARGUMENT);
            return 2;
        }
        if (0 == lua_isinteger(L, -1)) {
            lua_pushnil(L);
            lua_pushinteger(L, TUP_PARSER_ERROR_INVALID_ARGUMENT);
            return 2;
        }
        lua_Integer tmp_size = luaL_checkinteger(L, -1);
        if (tmp_size < 0) {
            lua_pushnil(L);
            lua_pushinteger(L, TUP_PARSER_ERROR_OUT_OF_RANGE);
            return 2;
        }
        size = (uint64_t)tmp_size;
    }

    if (0 == size || range.raw_size - offset < size) {
        lua_pushnil(L);
        lua_pushinteger(L, TUP_PARSER_ERROR_OUT_OF_RANGE);
        return 2;
    }

    char *buf = (char *)malloc(size);
    if (NULL == buf) {
        lua_pushnil(L);
        lua_pushinteger(L, TUP_PARSER_ERROR_CANNOT_ALLOCATE_MEMORY);
        return 2;
    }

    tup_get_ipkg_param param = {};
    param.flags = flags;
    param.offset = offset;
    param.buffer_size = size;
    param.buffer = buf;
    ret = tup_get_inner_package_data(ctx->parser, index, &param);
    if (0 != ret) {
        free(buf);
        lua_pushnil(L);
        lua_pushinteger(L, ret);
        return 2;
    }
    lua_pushlstring(L, buf, param.buffer_size);
    free(buf);

    return 1;
}

/* clang-format off */
/***
Gets inner package size of the given inner package index.
@function get_inner_package_size
@tparam integer index (should be within 1 and <i>get_inner_package_count()</i>)
@treturn table range
<p> has following field as size information of inner package </p>
  <p> - <span class="parameter">offset</span> (<b><i>integer</i></b>) Offset value to the inner package</p>
  <p> - <span class="parameter">size</span> (<b><i>integer</i></b>) Size of the inner package</p>
  <p> - <span class="parameter">raw_size</span> (<b><i>integer</i></b>) Original size of the inner package if compressed, otherwise same as <i>size</i></p>
@treturn userdata error object
@usage
local tup_parser = require("tup_parser")
local parser, err = tup_parser.from_file("/path/to/file")
local range, size
range, err = parser:get_inner_package_size(1)
size = range.size
*/
/* clang-format on */
static int
get_inner_package_size(lua_State *L)
{
    context *ctx = context_check(L, 1);
    if (NULL == ctx) {
        lua_pushnil(L);
        lua_pushinteger(L, TUP_PARSER_ERROR_INVALID_META_CONTEXT);
        return 2;
    }

    if (0 == lua_isinteger(L, 2)) {
        lua_pushnil(L);
        lua_pushinteger(L, TUP_PARSER_ERROR_INVALID_ARGUMENT);
        return 2;
    }
    lua_Integer index = luaL_checkinteger(L, 2) - 1;
    if (index < 0 || UINT32_MAX < index) {
        lua_pushnil(L);
        lua_pushinteger(L, TUP_PARSER_ERROR_OUT_OF_RANGE);
        return 2;
    }

    tup_get_range range;
    int ret = tup_get_inner_package_size(ctx->parser, index, &range);
    if (0 != ret) {
        lua_pushnil(L);
        lua_pushinteger(L, ret);
        return 2;
    }
    lua_newtable(ctx->L);
    lua_pushinteger(ctx->L, range.offset);
    lua_setfield(ctx->L, -2, "offset");
    lua_pushinteger(ctx->L, range.size);
    lua_setfield(ctx->L, -2, "size");
    lua_pushinteger(ctx->L, range.raw_size);
    lua_setfield(ctx->L, -2, "raw_size");

    return 1;
}

/* clang-format off */
/***
Verify patched data
@function verify_patched_data
@tparam string patched data
@tparam integer offset of patched data
@tparam userdata TLV object of ICV_AFTER_DATA
@treturn userdata error object (Succeeded: nil, Failed: not nil)
@usage
local tup_parser = require("tup_parser")
local parser, err = tup_parser.from_file("/path/to/file")
local ipkginfo
ipkginfo, err = parser:get_inner_package_info(1)
local tlvs
tlvs, err = parser:get_tlv({id=tup_parser.TLV_ID.ICV_AFTER_DATA, parent_tlv=ipkginfo})
local patched_data = "..."
local offset = 0
err = parser:verify_patched_data(patched_data, offset, tlvs[1])
*/
/* clang-format on */
static int
verify_patched_data(lua_State *L)
{
    context *ctx = context_check(L, 1);
    if (NULL == ctx) {
        lua_pushinteger(L, TUP_PARSER_ERROR_INVALID_META_CONTEXT);
        return 1;
    }

    if (0 == lua_isstring(L, 2)) {
        lua_pushinteger(L, TUP_PARSER_ERROR_INVALID_ARGUMENT);
        return 1;
    }

    if (0 == lua_isinteger(L, 3)) {
        lua_pushinteger(L, TUP_PARSER_ERROR_INVALID_ARGUMENT);
        return 1;
    }

    if (0 == lua_isuserdata(L, 4)) {
        lua_pushinteger(L, TUP_PARSER_ERROR_INVALID_ARGUMENT);
        return 1;
    }

    size_t len;
    const char *patched_data = luaL_tolstring(L, 2, &len);

    lua_Integer tmp_offset = luaL_checkinteger(L, 3);
    if (tmp_offset < 0) {
        lua_pushinteger(L, TUP_PARSER_ERROR_OUT_OF_RANGE);
        return 1;
    }
    uint64_t offset = (uint64_t)tmp_offset;

    tlv_context *tlv_ctx = tlv_context_check(L, 4);
    if (NULL == tlv_ctx) {
        lua_pushinteger(L, TUP_PARSER_ERROR_INVALID_META_CONTEXT);
        return 1;
    }

    int ret = tup_verify_patched_data(
      ctx->parser, (const uint8_t *)patched_data, len, offset, tlv_ctx->tlv);
    if (0 != ret) {
        lua_pushinteger(L, ret);
        return 1;
    }

    return 0;
}

/* clang-format off */
/***
Gets the type value of TLV object which has 2 bytes length with a following format.
<table style="width: 90%;" border="1">
  <tbody>
    <tr>
        <th>bit</th> <th>0</th> <th>1</th> <th>2</th> <th>3</th>
        <th>4</th> <th>5</th> <th>6</th> <th>7</th> <th>8</th> <th>9</th>
        <th>10</th> <th>11</th> <th>12</th> <th>13</th> <th>14</th> <th>15</th>
    </tr>
    <tr>
        <td>content</td> <td>N</td> <td>R</td> <td>R</td> <td>R</td>
        <td>I</td> <td>I</td> <td>I</td> <td>I</td> <td>I</td> <td>I</td>
        <td>I</td> <td>I</td> <td>V</td> <td>V</td> <td>V</td> <td>X</td>
    </tr>
  </tbody>
</table>
<p> - <span class="parameter">N</span> Namespace (1 bit) </p>
<p> - <span class="parameter">R</span> Reserved (3 bits) </p>
<p> - <span class="parameter">I</span> TLV ID (8 bits) (function @{get_id} is provided to get this value) </p>
<p> - <span class="parameter">V</span> Version (3 bits) </p>
<p> - <span class="parameter">X</span> Type-dependent bit (The usage is determined by the ID) (1 bit) </p>
@function get_type
@treturn integer type of TLV
@usage
local tup_parser = require("tup_parser")
local parser, err = tup_parser.from_file("/path/to/file")
local tlvs
tlvs, err = parser:get_tlv({id=tup_parser.TLV_ID.UPDATEFLOW})
local type = tlvs[1]:get_type()
*/
/* clang-format on */
static int
get_type(lua_State *L)
{
    tlv_context *tlv_ctx = tlv_context_check(L, 1);
    if (NULL == tlv_ctx) {
        lua_pushnil(L);
        lua_pushinteger(L, TUP_PARSER_ERROR_INVALID_META_CONTEXT);
        return 2;
    }

    uint16_t type;
    int ret = tup_get_tlv_type(tlv_ctx->tlv, &type);
    if (0 != ret) {
        lua_pushnil(L);
        lua_pushinteger(L, ret);
        return 2;
    }

    lua_pushinteger(L, type);
    return 1;
}

/* clnag-format off */
/***
Gets the ID of TLV object which is defined in type value of TLV object.
@function get_id
@treturn integer TLV ID (either field in @{TLV_ID})
@usage
local tup_parser = require("tup_parser")
local parser, err = tup_parser.from_file("/path/to/file")
local tlvs
tlvs, err = parser:get_tlv({id=tup_parser.TLV_ID.UPDATEFLOW})
local id = tlvs[1]:get_id()
*/
/* clang-format on */
static int
get_id(lua_State *L)
{
    tlv_context *tlv_ctx = tlv_context_check(L, 1);
    if (NULL == tlv_ctx) {
        lua_pushnil(L);
        lua_pushinteger(L, TUP_PARSER_ERROR_INVALID_META_CONTEXT);
        return 2;
    }

    uint16_t type;
    int ret = tup_get_tlv_type(tlv_ctx->tlv, &type);
    if (0 != ret) {
        lua_pushnil(L);
        lua_pushinteger(L, ret);
        return 2;
    }

    lua_pushinteger(L, type & TUP_ID_MASK_T);
    return 1;
}

/* clang-format off */
/***
Gets the length of TLV object.
@function get_length
@treturn integer length of TLV
@usage
local tup_parser = require("tup_parser")
local parser, err = tup_parser.from_file("/path/to/file")
local tlvs
tlvs, err = parser:get_tlv({id=tup_parser.TLV_ID.UPDATEFLOW})
local length = tlvs[1]:get_length()
*/
/* clang-format on */
static int
get_length(lua_State *L)
{
    tlv_context *tlv_ctx = tlv_context_check(L, 1);
    if (NULL == tlv_ctx) {
        lua_pushnil(L);
        lua_pushinteger(L, TUP_PARSER_ERROR_INVALID_META_CONTEXT);
        return 2;
    }

    uint64_t length;
    int ret = tup_get_tlv_length(tlv_ctx->tlv, &length);
    if (0 != ret) {
        lua_pushnil(L);
        lua_pushinteger(L, ret);
        return 2;
    }

    lua_pushinteger(L, (lua_Integer)length);
    return 1;
}

/* clang-format off */
/***
Gets the value size of TLV object.
@function get_value_size
@treturn integer size of value in TLV
@usage
local tup_parser = require("tup_parser")
local parser, err = tup_parser.from_file("/path/to/file")
local tlvs
tlvs, err = parser:get_tlv({id=tup_parser.TLV_ID.UPDATEFLOW})
local size = tlvs[1]:get_value_size()
*/
/* clang-format on */
static int
get_value_size(lua_State *L)
{
    tlv_context *tlv_ctx = tlv_context_check(L, 1);
    if (NULL == tlv_ctx) {
        lua_pushnil(L);
        lua_pushinteger(L, TUP_PARSER_ERROR_INVALID_META_CONTEXT);
        return 2;
    }

    uint64_t size;
    int ret = tup_get_tlv_value_size(tlv_ctx->tlv, &size);
    if (0 != ret) {
        lua_pushnil(L);
        lua_pushinteger(L, ret);
        return 2;
    }

    lua_pushinteger(L, (lua_Integer)size);
    return 1;
}

/* clang-format off */
/***
Gets the value of TLV object.
@function get_value
@tparam table args
  <p> - <span class="parameter">offset (optional)</span> (<b><i>integer</i></b>)</p>
  <p> - <span class="parameter">size   (optional)</span> (<b><i>integer</i></b>)</p>
@treturn string value of TLV
@treturn userdata error object
@usage
local tup_parser = require("tup_parser")
local parser, err = tup_parser.from_file("/path/to/file")
local tlvs
tlvs, err = parser:get_tlv({id=tup_parser.TLV_ID.UPDATEFLOW})
local data
data, err = tlvs[1]:get_value()
*/
/* clang-format on */
static int
get_value(lua_State *L)
{
    tlv_context *tlv_ctx = tlv_context_check(L, 1);
    if (NULL == tlv_ctx) {
        lua_pushnil(L);
        lua_pushinteger(L, TUP_PARSER_ERROR_INVALID_META_CONTEXT);
        return 2;
    }

    uint64_t value_size;
    int ret = tup_get_tlv_value_size(tlv_ctx->tlv, &value_size);
    if (0 != ret) {
        lua_pushnil(L);
        lua_pushinteger(L, ret);
        return 2;
    }

    uint64_t offset, size;
    if (0 == lua_istable(L, 2)) {
        offset = 0;
        size = value_size;
    } else {
        int type = lua_getfield(L, -1, "offset");
        if (LUA_TNIL == type) {
            offset = 0;
        } else if (LUA_TNUMBER == type) {
            if (0 == lua_isinteger(L, -1)) {
                lua_pushnil(L);
                lua_pushinteger(L, TUP_PARSER_ERROR_INVALID_ARGUMENT);
                return 2;
            }
            lua_Integer tmp_offset = luaL_checkinteger(L, -1);
            if (tmp_offset < 0 || TUP_TLV_LENGTH_MAX < tmp_offset) {
                lua_pushnil(L);
                lua_pushinteger(L, TUP_PARSER_ERROR_OUT_OF_RANGE);
                return 2;
            }
            offset = (uint64_t)tmp_offset;
        } else {
            lua_pushnil(L);
            lua_pushinteger(L, TUP_PARSER_ERROR_INVALID_ARGUMENT);
            return 2;
        }

        type = lua_getfield(L, -2, "size");
        if (LUA_TNIL == type) {
            size = value_size - offset;
        } else if (LUA_TNUMBER == type) {
            if (0 == lua_isinteger(L, -1)) {
                lua_pushnil(L);
                lua_pushinteger(L, TUP_PARSER_ERROR_INVALID_ARGUMENT);
                return 2;
            }
            lua_Integer tmp_size = luaL_checkinteger(L, -1);
            if (tmp_size < 0 || TUP_TLV_LENGTH_MAX < tmp_size) {
                lua_pushnil(L);
                lua_pushinteger(L, TUP_PARSER_ERROR_OUT_OF_RANGE);
                return 2;
            }
            size = (uint64_t)tmp_size;
        } else {
            lua_pushnil(L);
            lua_pushinteger(L, TUP_PARSER_ERROR_INVALID_ARGUMENT);
            return 2;
        }
    }

    if (0 == size || value_size < offset || (value_size - offset) < size) {
        lua_pushnil(L);
        lua_pushinteger(L, TUP_PARSER_ERROR_OUT_OF_RANGE);
        return 2;
    }

    char *buf = (char *)malloc(size);
    if (NULL == buf) {
        lua_pushnil(L);
        lua_pushinteger(L, TUP_PARSER_ERROR_CANNOT_ALLOCATE_MEMORY);
        return 2;
    }

    ret = tup_get_tlv_value(tlv_ctx->tlv, offset, buf, size);
    if (0 != ret) {
        free(buf);
        lua_pushnil(L);
        lua_pushinteger(L, ret);
        return 2;
    }

    lua_pushlstring(L, buf, size);
    free(buf);
    return 1;
}

/* clang-format off */
/***
Gets delta patch information
@function get_delta_info
@treturn table delta patch information
<p> has following field as delta patch information </p>
  <p> - <span class="parameter">algorithm</span> (<b><i>integer</i></b>) Delta patch algorithm</p>
  <p> - <span class="parameter">version</span> (<b><i>integer</i></b>) Delta patch algorithm version</p>
  <p> - <span class="parameter">presize</span> (<b><i>integer</i></b>) Pre-patch data size</p>
  <p> - <span class="parameter">postsize</span> (<b><i>integer</i></b>) Post-patch data size</p>
  <p> - <span class="parameter">attr (optional)</span> (<b><i>string</i></b>) Algorithm specific attribution</p>
@treturn userdata error object
@usage
local tup_parser = require("tup_parser")
local parser, err = tup_parser.from_file("/path/to/file")
local ipkginfo
ipkginfo, err = parser:get_inner_package_info(1)
local tlvs
tlvs, err = parser:get_tlv({id=tup_parser.TLV_ID.DELTA_PATCH, parent_tlv=ipkginfo})
local deltainfo
deltainfo, err = tlvs[1]:get_delta_info()
*/
/* clang-format on */
static int
get_delta_info(lua_State *L)
{
    tlv_context *tlv_ctx = tlv_context_check(L, 1);
    if (NULL == tlv_ctx) {
        lua_pushnil(L);
        lua_pushinteger(L, TUP_PARSER_ERROR_INVALID_META_CONTEXT);
        return 2;
    }

    uint16_t type;
    int ret = tup_get_tlv_type(tlv_ctx->tlv, &type);
    if (0 != ret) {
        lua_pushnil(L);
        lua_pushinteger(L, ret);
        return 2;
    }
    if (TUP_ID_DELTA_PATCH != (type & TUP_ID_MASK_T)) {
        lua_pushnil(L);
        lua_pushinteger(L, TUP_PARSER_ERROR_MISMATCH_TLV_ID);
        return 2;
    }

    uint64_t size;
    ret = tup_get_tlv_value_size(tlv_ctx->tlv, &size);
    if (0 != ret) {
        lua_pushnil(L);
        lua_pushinteger(L, ret);
        return 2;
    }

    struct delta_patch {
        uint32_t algorithm;
        uint32_t version;
        uint64_t presize;
        uint64_t postsize;
        char attr[];
    };

    if (sizeof(struct delta_patch) > size) {
        lua_pushnil(L);
        lua_pushinteger(L, TUP_PARSER_ERROR_SMALL_DATA_SIZE);
        return 2;
    }

    struct delta_patch *delta = (struct delta_patch *)malloc(size);
    if (NULL == delta) {
        lua_pushnil(L);
        lua_pushinteger(L, TUP_PARSER_ERROR_CANNOT_ALLOCATE_MEMORY);
        return 2;
    }

    ret = tup_get_tlv_value(tlv_ctx->tlv, 0, delta, size);
    if (0 != ret) {
        free(delta);
        lua_pushnil(L);
        lua_pushinteger(L, ret);
        return 2;
    }
    // TODO: endian convert

    lua_newtable(L);
    lua_pushinteger(L, delta->algorithm);
    lua_setfield(L, -2, "algorithm");
    lua_pushinteger(L, delta->version);
    lua_setfield(L, -2, "version");
    lua_pushinteger(L, delta->presize);
    lua_setfield(L, -2, "presize");
    lua_pushinteger(L, delta->postsize);
    lua_setfield(L, -2, "postsize");
    if (sizeof(struct delta_patch) != size) {
        lua_pushlstring(L, delta->attr, size - sizeof(struct delta_patch));
        lua_setfield(L, -2, "attr");
    }
    free(delta);

    return 1;
}

/* variable */
static const struct luaL_Reg parser_context_methods[] = {
    { "validate", validate }, { "get_tlv", get_tlv },
    { "get_inner_package_info", get_inner_package_info },
    { "get_inner_package_data", get_inner_package_data },
    { "get_inner_package_count", get_inner_package_count },
    { "get_inner_package_size", get_inner_package_size },
    { "verify_patched_data", verify_patched_data }, { "__gc", cleanup },
    { NULL, NULL }
};

static const struct luaL_Reg tlv_context_methods[] = { { "get_type", get_type },
    { "get_length", get_length }, { "get_value_size", get_value_size },
    { "get_value", get_value }, { "get_id", get_id },
    { "get_delta_info", get_delta_info }, { NULL, NULL } };

/* function */
int
luaopen_tup_parser(lua_State *L)
{
    luaL_newmetatable(L, TUP_PARSER_META_CONTEXT);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, parser_context_methods, 0);

    luaL_newmetatable(L, TUP_TLV_META_CONTEXT);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, tlv_context_methods, 0);

    luaopen_tup_parser_wrapper_lib(L);
    return 1;
}
