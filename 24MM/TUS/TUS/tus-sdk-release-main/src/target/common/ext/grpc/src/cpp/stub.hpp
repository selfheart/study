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

#ifndef TMC_GRPC_STUB_H
#define TMC_GRPC_STUB_H

#include <lua.hpp>
#include <cstring>
#include <iostream>

#include <grpc/grpc.h>
#include <grpc/support/alloc.h>
#include <grpc/byte_buffer.h>        // for grpc_byte_buffer_reader_init()
#include <grpc/byte_buffer_reader.h> // for grpc_byte_buffer_reader

#define GRPC_STUB_META_CONTEXT "grpc.stub.context"
#include "channel.hpp"
#include "grpc_batch_context.hpp"
#include "grpc_wrap_utility.hpp"

#endif
