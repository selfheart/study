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
#include "protobuf_char_wrapper.hpp"

#include "protobuf_helper.hpp"

int tmc_protobuf_char_eq(lua_State *L)
    noexcept
{
    char *a = tmc_protobuf_lua_check_char_ptr(L, 1);
    char *b = tmc_protobuf_lua_check_char_ptr(L, 2);
    tmc_protobuf_lua_push_bool(L, a == b);
    return 1;
}

int tmc_protobuf_char_add(lua_State *L)
    noexcept
{
    char *a = tmc_protobuf_lua_check_char_ptr(L, 1);
    long long b = tmc_protobuf_lua_check_integer(L, 2);
    tmc_protobuf_lua_push_char_ptr(L, a + b);
    return 1;
}

int tmc_protobuf_char_sub(lua_State *L)
    noexcept
{
    char *a = tmc_protobuf_lua_check_char_ptr(L, 1);
    long long b = tmc_protobuf_lua_check_integer(L, 2);
    tmc_protobuf_lua_push_char_ptr(L, a - b);
    return 1;
}
