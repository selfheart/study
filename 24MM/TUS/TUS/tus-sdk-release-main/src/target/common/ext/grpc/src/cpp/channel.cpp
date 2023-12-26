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

#include "channel.hpp"
#include "grpc/grpc_security.h"


extern "C" {
// workaround for Ubuntu 22.04's missing __wrap_memcpy() problem
#include <string.h>
void __attribute__((weak)) *__wrap_memcpy(void *dest, const void *src, size_t n)
{
        return (memmove(dest, src, n));
}
}


static int tmc_new_channel(lua_State *L)
{
  const char *hostname = luaL_checkstring(L, 1);

  channel_context *ctx = (channel_context *)lua_newuserdata(L, sizeof(channel_context));

  // fixme:
  // workaround for throwing exception of gRPC core library APIs.
  try
  {
#ifdef GRPC_NO_INSECURE
      grpc_channel_credentials * insecure_cred = grpc_insecure_credentials_create();
      ctx->channel = grpc_channel_create(hostname, insecure_cred, nullptr);
      grpc_channel_credentials_release(insecure_cred);
#else
      ctx->channel = grpc_insecure_channel_create(hostname, nullptr, nullptr);
#endif
  }
  catch(...)
  {
    return luaL_error(L,"unknown error occurred: %s\n",__func__);
  }

  luaL_getmetatable(L, GRPC_CHANNEL_META_CONTEXT);
  lua_setmetatable(L, -2);

  return 1;
}

static void luaopen_grpc_channel_lib(lua_State *L)
{
  static const struct luaL_Reg grpc_channel_lib[] = {
      {"new", tmc_new_channel},
      {NULL, NULL}};
  luaL_newlib(L, grpc_channel_lib);
  return;
}

extern "C"
{
  int luaopen_tmc_wrap_grpc_channel(lua_State *L)
  {
    // create meta table.
    luaL_newmetatable(L, GRPC_CHANNEL_META_CONTEXT);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaopen_grpc_channel_lib(L);

    return 1;
  }
}
