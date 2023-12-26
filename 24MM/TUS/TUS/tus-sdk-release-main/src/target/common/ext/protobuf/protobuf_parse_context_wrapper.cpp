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
/*
@module tmc_protobuf
*/
#include "protobuf_parse_context_wrapper.hpp"

#include "protobuf_message_lite.hpp"
#include "protobuf_helper.hpp"

#include <google/protobuf/parse_context.h>
#include <google/protobuf/stubs/port.h>

using google::protobuf::uint8;
using google::protobuf::internal::ParseContext;

/*
Wrapper of google::protobuf::internal::ParseContext::Done.
<pre><code>
bool google::protobuf::internal::ParseContext::Done(const char **ptr)
</code></pre>
@function parse_context.done(ptr)
@tparam char* ptr
@treturn boolean return value of the wrapped function
@usage ret = ctx:done(ptr)
*/
int tmc_protobuf_parse_context_done(lua_State *L)
{
    ParseContext *self = tmc_protobuf_lua_check_parse_context(L, 1);
    const char **ptr = (const char **)tmc_protobuf_lua_check_char_ptr_ptr(L, 2);
    bool b = self->Done(ptr);
    tmc_protobuf_lua_push_bool(L, b);
    return 1;
}

/*
Wrapper of google::protobuf::internal::ParseContext::ParseMessage.
<pre><code>
template <typename T> const char *
    google::protobuf::internal::ParseContext::ParseMessage(
        T *msg, const char *ptr)
</code></pre>
@function parse_context.parse_message(msg, ptr)
@tparam MessageLite msg
@tparam char* ptr
@treturn char* return value of the wrapped function
@usage ptr = ctx:parse_message(message, ptr)
*/
int tmc_protobuf_parse_context_parse_message(lua_State *L)
{
    ParseContext *self = tmc_protobuf_lua_check_parse_context(L, 1);
    MessageLiteLua *msg = tmc_protobuf_lua_check_message_lite(L, 2);
    const char *ptr = tmc_protobuf_lua_check_char_ptr(L, 3);
    ptr = self->ParseMessage(msg, ptr);
    tmc_protobuf_lua_push_char_ptr(L, ptr);
    return 1;
}

/*
Wrapper of google::protobuf::internal::ParseContext::DataAvailable.
<pre><code>
bool google::protobuf::internal::EpsCopyInputStream::DataAvailable(const char *ptr)
</code></pre>
Returns true if more data is available, if false is returned one has to call Done for further checks.
@function parse_context.data_available(ptr)
@tparam char* ptr
@treturn boolean return value of the wrapped function
@usage ptr = ctx:data_available(ptr)
*/
int tmc_protobuf_parse_context_data_available(lua_State *L)
{
    ParseContext *self = tmc_protobuf_lua_check_parse_context(L, 1);
    const char *ptr = tmc_protobuf_lua_check_char_ptr(L, 2);
    bool b = self->DataAvailable(ptr);
    tmc_protobuf_lua_push_bool(L, b);
    return 1;
}
