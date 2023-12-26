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

#ifndef TMC_GRPC_CHANNEL_H
#define TMC_GRPC_CHANNEL_H

#include <lua.hpp>
#include <grpc/grpc.h>

typedef struct channel_context
{
  struct grpc_channel *channel;
} channel_context;

#define GRPC_CHANNEL_META_CONTEXT "grpc.channel.context"

#endif
