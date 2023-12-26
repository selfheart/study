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

#ifndef TMC_GRPC_UTILITY_H
#define TMC_GRPC_UTILITY_H

#include <lua.hpp>
#include <cstring>
#include <iostream>
#include <sstream>

#include <grpc/grpc.h>
#include <grpc/support/alloc.h>
#include <grpc/byte_buffer.h>        // for grpc_byte_buffer_reader_init()
#include <grpc/byte_buffer_reader.h> // for grpc_byte_buffer_reader

grpc_slice conv_buf2slice(grpc_byte_buffer *buf);

gpr_timespec create_timespec_from_millis(int64_t timeout_millis);

gpr_timespec create_timespec_infinite();

gpr_timespec handle_optional_timeout_parameter(lua_State *L, int index);

#endif
