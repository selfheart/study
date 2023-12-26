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

#include "grpc_batch_context.hpp"

/*
 * create and init grpcwrap_batch_context
 */
grpcwrap_batch_context *grpcwrap_batch_context_create()
{
  auto *ctx =
      (grpcwrap_batch_context *)gpr_malloc(sizeof(grpcwrap_batch_context));
  memset(ctx, 0, sizeof(grpcwrap_batch_context));
  return ctx;
}
