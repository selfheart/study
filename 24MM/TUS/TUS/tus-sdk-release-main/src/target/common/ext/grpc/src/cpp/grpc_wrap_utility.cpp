/* TMC CONFIDENTIAL
 * $TUSLibId$
 * Copyright (C) 2022 TOYOTA MOTOR CORPORATION
 * All Rights Reserved.
 */
/* TMC CONFIDENTIAL
* $JITDFLibId$
* Copyright (C) 2020 TOYOTA MOTOR CORPORATION
* All Rights Reserved.
*/

#include "grpc_wrap_utility.hpp"

/*
 * helper function
 */
grpc_slice conv_buf2slice(grpc_byte_buffer *buf)
{
  grpc_byte_buffer_reader bbr;
  grpc_byte_buffer_reader_init(&bbr, buf);
  grpc_slice slice = grpc_byte_buffer_reader_readall(&bbr);
  grpc_byte_buffer_reader_destroy(&bbr);
  return slice;
}

gpr_timespec create_timespec_from_millis(int64_t timeout_millis)
{
  return gpr_time_add(gpr_now(GPR_CLOCK_MONOTONIC), gpr_time_from_millis(timeout_millis, GPR_TIMESPAN));
}

gpr_timespec create_timespec_infinite()
{
  return gpr_inf_future(GPR_CLOCK_MONOTONIC);
}

gpr_timespec handle_optional_timeout_parameter(lua_State *L, int index)
{
  struct gpr_timespec timeout;
  if (lua_isnoneornil(L, index) == 1)
  {
    // if parameter timeout is not specified
    timeout = create_timespec_infinite();
  }
  else
  {
    lua_Integer timeout_millis = luaL_checkinteger(L, index);
    if (timeout_millis < 0 || timeout_millis > INT64_MAX)
    {
      luaL_error(L, "parameter timeout should be positive integer to ", INT64_MAX);
    }
    timeout = create_timespec_from_millis(timeout_millis);
  }
  return timeout;
}
