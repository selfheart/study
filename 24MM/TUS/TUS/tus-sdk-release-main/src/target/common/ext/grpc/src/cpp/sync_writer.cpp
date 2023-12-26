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
#include "sync_writer.hpp"

typedef struct
{
  lua_State *L;
  struct grpc_channel *channel;
  struct grpc_completion_queue *queue;
  struct grpc_call *call;
  grpcwrap_batch_context *batch_context;
} sync_writer_context;

static sync_writer_context *context_check(lua_State *L, int i)
{
  return (sync_writer_context *)luaL_checkudata(L, i, GRPC_SYNC_WRITER_META_CONTEXT);
}

/*
 * only start batch GRPC_OP_SEND_INITIAL_METADATA
 */
grpc_call_error grpcwrap_call_start_client_streaming(
    grpc_call *call, grpcwrap_batch_context *ctx,
    uint32_t initial_metadata_flags,
    void *tag)
{
  /* TODO: don't use magic number */
  grpc_op ops[1];
  grpc_op *op;

  memset(ops, 0, sizeof(ops));
  op = ops;
  op->op = GRPC_OP_SEND_INITIAL_METADATA;
  op->data.send_initial_metadata.count = ctx->send_initial_metadata.count;
  op->data.send_initial_metadata.metadata =
      ctx->send_initial_metadata.metadata;
  op->flags = initial_metadata_flags;
  op->reserved = nullptr;

  return grpc_call_start_batch(call, ops, sizeof(ops) / sizeof(ops[0]), tag,
                               nullptr);
}

/*
 * only start batch GRPC_OP_SEND_MESSAGE to do client streaming.
 */
grpc_call_error grpcwrap_call_write_message(grpc_call *call, grpc_slice *send_buffer, uint32_t write_flags, void *tag)
{
  grpc_op ops[1];
  grpc_op *op;

  memset(ops, 0, sizeof(ops));
  op = ops;
  op->op = GRPC_OP_SEND_MESSAGE;
  ops->data.send_message.send_message = grpc_raw_byte_buffer_create(send_buffer, 1);
  ops->flags = write_flags;
  ops->reserved = nullptr;

  return grpc_call_start_batch(call, ops, sizeof(ops) / sizeof(ops[0]), tag,
                               nullptr);
}

/*
 * start batch to close and receive response from server.
 */
grpc_call_error grpcwrap_call_close(grpc_call *call, grpcwrap_batch_context *ctx, void *tag)
{
  grpc_op ops[4];
  grpc_op *op;
  memset(ops, 0, sizeof(ops));

  op = ops;
  op->op = GRPC_OP_SEND_CLOSE_FROM_CLIENT;
  op->flags = 0;
  op->reserved = nullptr;

  op++;
  op->op = GRPC_OP_RECV_INITIAL_METADATA;
  op->data.recv_initial_metadata.recv_initial_metadata =
      &(ctx->recv_initial_metadata);
  op->flags = 0;
  op->reserved = nullptr;

  op++;
  op->op = GRPC_OP_RECV_MESSAGE;
  op->data.recv_message.recv_message = &(ctx->recv_message);
  op->flags = 0;
  op->reserved = nullptr;

  op++;
  op->op = GRPC_OP_RECV_STATUS_ON_CLIENT;
  op->data.recv_status_on_client.trailing_metadata =
      &(ctx->recv_status_on_client.trailing_metadata);
  op->data.recv_status_on_client.status =
      &(ctx->recv_status_on_client.status);
  op->data.recv_status_on_client.status_details =
      &(ctx->recv_status_on_client.status_details);
  op->data.recv_status_on_client.error_string = &(ctx->recv_status_on_client.error_string);
  op->flags = 0;
  op->reserved = nullptr;

  return grpc_call_start_batch(call, ops, sizeof(ops) / sizeof(ops[0]), tag,
                               nullptr);
}

/*
 * create completion queue and grpc_call for methods open,write,close.
 */
static int tmc_new(lua_State *L)
{
  channel_context *channel = (channel_context *)luaL_checkudata(L, 1, GRPC_CHANNEL_META_CONTEXT);
  const char *rpc_method = luaL_checkstring(L, 2);

  struct gpr_timespec timeout = handle_optional_timeout_parameter(L, 3);

  sync_writer_context *ctx = (sync_writer_context *)lua_newuserdata(L, sizeof(sync_writer_context));
  ctx->channel = channel->channel;
  ctx->L = L;

  struct grpc_completion_queue *queue = grpc_completion_queue_create_for_pluck(nullptr);
  struct grpc_slice method = grpc_slice_from_static_string(rpc_method);

  // if parent_call is NULL, because this is grpc client library.
  grpc_call *call = grpc_channel_create_call(ctx->channel, nullptr, GRPC_PROPAGATE_DEFAULTS, queue, method, nullptr, timeout, nullptr);
  grpcwrap_batch_context *batch_context = grpcwrap_batch_context_create();
  ctx->call = call;
  ctx->queue = queue;
  ctx->batch_context = batch_context;

  luaL_getmetatable(L, GRPC_SYNC_WRITER_META_CONTEXT);
  lua_setmetatable(L, -2);

  return 1;
}

static void clean(sync_writer_context *ctx)
{
  // clean up
  if (ctx->queue != NULL)
  {
    grpc_completion_queue_shutdown(ctx->queue);
    grpc_completion_queue_destroy(ctx->queue);
    ctx->queue = NULL;
  }
  if (ctx->batch_context != NULL)
  {
    gpr_free(ctx->batch_context);
    ctx->batch_context = NULL;
  }
}

static int tmc_open(lua_State *L)
{
#ifdef DEBUG
  std::cout << "tmc_open called." << std::endl;
#endif

  sync_writer_context *ctx = context_check(L, 1);
  struct gpr_timespec deadline = create_timespec_infinite();

  if (ctx->queue == NULL)
  {
    return luaL_error(L, "it should be called after calling new()\n");
  }

  grpc_call_error error;
  // fixme:
  // workaround for throwing exception of gRPC core library APIs.
  try
  {
    error = grpcwrap_call_start_client_streaming(ctx->call, ctx->batch_context, 0, ctx);
  }
  catch(...)
  {
    return luaL_error(L,"unknown error occurred: %s\n",__func__);
  }

  // call batch
  // TODO: after supporting metadata feature, handling metadata is necessary.
  if (error != GRPC_CALL_OK)
  {
    return luaL_error(L, "cannot start batch. grpc_call_error rc is %d\n", error);
  }

  // fixme:
  // workaround for throwing exception of gRPC core library APIs.
  grpc_event event;
  try
  {
    // block until response arrived.
    event = grpc_completion_queue_pluck(ctx->queue, ctx, deadline, nullptr);
  }
  catch(...)
  {
    return luaL_error(L,"unknown error occurred: %s\n",__func__);
  }

  // only if event is failure, return extended error code for client to call close() to know detail return code.
  if (event.type == GRPC_OP_COMPLETE && event.success == 0)
  {
    // something wrong occurs. for example server does not exist.
    lua_pushinteger(L, GRPC_OP_COMPLETE_WITH_FAILURE);
    return 1;
  }

  lua_pushinteger(L, event.type);
  return 1;
}

/*
 * send message
 */
static int tmc_write(lua_State *L)
{
  // get payload and timeout
  sync_writer_context *ctx = context_check(L, 1);
  const char *encoded_payload = luaL_checkstring(L, 2);
  struct gpr_timespec deadline = create_timespec_infinite();

  if (ctx->queue == NULL)
  {
    return luaL_error(L, "it should be called after calling new()\n");
  }

  // fixme:
  // workaround for throwing exception of gRPC core library APIs.
  try
  {
    struct grpc_slice payload = grpc_slice_from_static_string(encoded_payload);

    // create grpc_op and start batch of GRPC_OP_SEND_MESSAGE
    grpc_call_error error = grpcwrap_call_write_message(ctx->call, &payload, 0, ctx);
    if (error != GRPC_CALL_OK)
    {
      return luaL_error(L, "cannot start batch. grpc_call_error rc is %d\n", error);
    }

    // block until response arrived.
    grpc_event event = grpc_completion_queue_pluck(ctx->queue, ctx, deadline, nullptr);

    // only if event is failure, return extended error code for client to call close() to know detail return code.
    if (event.type == GRPC_OP_COMPLETE && event.success == 0)
    {
      // something wrong occurs. for example server does not exist
      lua_pushinteger(L, GRPC_OP_COMPLETE_WITH_FAILURE);
      return 1;
    }

    lua_pushinteger(L, event.type);
    return 1;  
  }
  catch(...)
  {
    return luaL_error(L,"unknown error occurred: %s\n",__func__);
  }
}

/*
 * close and receive response from server.
 */
static int tmc_close(lua_State *L)
{
  // create grpc_op and start batch of GRPC_OP_SEND_CLOSE_FROM_CLIENT
  sync_writer_context *ctx = context_check(L, 1);
  struct gpr_timespec deadline = create_timespec_infinite();

  if (ctx->queue == NULL)
  {
    return luaL_error(L, "it should be called after calling new()\n");
  }

  // fixme:
  // workaround for throwing exception of gRPC core library APIs.
  try
  {
 
    // close and receive result data from server.
    grpc_call_error error = grpcwrap_call_close(ctx->call, ctx->batch_context, ctx);

    if (error != GRPC_CALL_OK)
    {
      return luaL_error(L, "cannot start batch. grpc_call_error rc is %d\n", error);
    }

    // block until response arrived.
    grpc_event event = grpc_completion_queue_pluck(ctx->queue, ctx, deadline, nullptr);

    // when only event.type is GRPC_QUEUE_SHUTDOWN, it should throw error because deadline is infinite.
    if (event.type != GRPC_OP_COMPLETE || (event.type == GRPC_OP_COMPLETE && event.success == 0))
    {
      return luaL_error(L, "cannot close correctly type:%d\n", event.type);
    }
  #ifdef DEBUG
    fprintf(stderr, "error string %s\n", ctx->batch_context->recv_status_on_client.error_string);
  #endif

    int ret_count = 1;
    lua_pushinteger(L, ctx->batch_context->recv_status_on_client.status);
    // return grpc return code and receive status
    if (ctx->batch_context->recv_status_on_client.status == GRPC_STATUS_OK)
    {
      grpc_slice req = conv_buf2slice(ctx->batch_context->recv_message);
      const char *decoded_response = grpc_slice_to_c_string(req);
      lua_pushstring(L, decoded_response);
      ret_count++;
      gpr_free((void *)decoded_response);
    }

    // clean up
    clean(ctx);
    return ret_count;
  }
  catch(...)
  {
    return luaL_error(L,"unknown error occurred: %s\n",__func__);
  }
}

static int tmc_cleanup(lua_State *L)
{
  sync_writer_context *ctx = context_check(L, 1);
  // clean up
  clean(ctx);

  return 0;
}

static void luaopen_grpc_channel_lib(lua_State *L)
{
  static const struct luaL_Reg grpc_sync_writer_lib[] = {
      {"new", tmc_new},
      {NULL, NULL}};
  luaL_newlib(L, grpc_sync_writer_lib);
  return;
}

static const struct luaL_Reg context_methods[] = {
    {"open", tmc_open},
    {"write", tmc_write},
    {"close", tmc_close},
    {"__gc", tmc_cleanup},
    {NULL, NULL}};

extern "C"
{
  int luaopen_tmc_wrap_grpc_sync_writer(lua_State *L)
  {
    // create meta table.
    luaL_newmetatable(L, GRPC_SYNC_WRITER_META_CONTEXT);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    luaL_setfuncs(L, context_methods, 0);
    luaopen_grpc_channel_lib(L);

    return 1;
  }
}
