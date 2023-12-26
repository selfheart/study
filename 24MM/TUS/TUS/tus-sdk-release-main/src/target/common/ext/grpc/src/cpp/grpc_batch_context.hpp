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

#ifndef TMC_BATCH_CONTEXT_H
#define TMC_BATCH_CONTEXT_H

#include <lua.hpp>
#include <cstring>
#include <iostream>

#include <grpc/grpc.h>
#include <grpc/support/alloc.h>
#include <grpc/byte_buffer.h>        // for grpc_byte_buffer_reader_init()
#include <grpc/byte_buffer_reader.h> // for grpc_byte_buffer_reader

/*
 * Helper to maintain grpc_call_start_batch inputs and store batch op outputs.
 */
typedef struct grpcwrap_batch_context
{
  grpc_metadata_array send_initial_metadata;
  grpc_byte_buffer *send_message;
  struct
  {
    grpc_metadata_array trailing_metadata;
  } send_status_from_server;
  grpc_metadata_array recv_initial_metadata;
  grpc_byte_buffer *recv_message;
  struct
  {
    grpc_metadata_array trailing_metadata;
    grpc_status_code status;
    grpc_slice status_details;
    const char *error_string;
  } recv_status_on_client;
  int recv_close_on_server_cancelled;
} grpcwrap_batch_context;

/**
 * create and initialize grpcwrap_batch_context.
 */
grpcwrap_batch_context *grpcwrap_batch_context_create();

#endif
