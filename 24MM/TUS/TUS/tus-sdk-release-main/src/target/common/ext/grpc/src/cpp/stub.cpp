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

#include "stub.hpp"

typedef struct
{
  lua_State *L;
  struct grpc_channel *channel;
} context;

static context *context_check(lua_State *L, int i)
{
  return (context *)luaL_checkudata(L, i, GRPC_STUB_META_CONTEXT);
}

/*
 * helper to execute unary call
 */
grpc_call_error grpcwrap_call_start_unary(
    grpc_call *call, grpcwrap_batch_context *ctx, grpc_slice *send_buffer,
    uint32_t write_flags,
    uint32_t initial_metadata_flags, void *tag)
{
  /* TODO: don't use magic number */
  grpc_op ops[6];
  memset(ops, 0, sizeof(ops));
  grpc_op *op = ops;
  op->op = GRPC_OP_SEND_INITIAL_METADATA;
  op->data.send_initial_metadata.count = ctx->send_initial_metadata.count;
  op->data.send_initial_metadata.metadata = ctx->send_initial_metadata.metadata;
  op->flags = initial_metadata_flags;
  op->reserved = nullptr;

  op++;
  op->op = GRPC_OP_SEND_MESSAGE;
  ctx->send_message = grpc_raw_byte_buffer_create(send_buffer, 1);
  op->data.send_message.send_message = ctx->send_message;
  op->flags = write_flags;
  op->reserved = nullptr;

  op++;
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
 * bind channel object
 */
static int tmc_new_grpc(lua_State *L)
{
  channel_context *channel = (channel_context *)luaL_checkudata(L, 1, GRPC_CHANNEL_META_CONTEXT);

  context *ctx = (context *)lua_newuserdata(L, sizeof(context));
  ctx->channel = channel->channel;
  ctx->L = L;

  luaL_getmetatable(L, GRPC_STUB_META_CONTEXT);
  lua_setmetatable(L, -2);

  return 1;
}

static void cleanup(grpc_completion_queue *queue, grpcwrap_batch_context *batch_context)
{
  // clean up
  if (queue != NULL)
  {
    grpc_completion_queue_shutdown(queue);
    grpc_completion_queue_destroy(queue);
  }
  if (batch_context != NULL)
  {
    gpr_free(batch_context);
  }
}

// TODO: will be replaced into grpcj_conv_buf2slice after integrating JITDF SDK 075
static int
internal_conv_buf2slice(grpc_byte_buffer *buf, grpc_slice *target)
{
    grpc_byte_buffer_reader bbr;
    // fixme:
    // workaround for throwing exception of gRPC core library APIs.
    try {
        int ret = grpc_byte_buffer_reader_init(&bbr, buf);
        if (ret != 1) {
            return -1;
        }
        *target = grpc_byte_buffer_reader_readall(&bbr);
        grpc_byte_buffer_reader_destroy(&bbr);
    } catch (...) {
        grpc_byte_buffer_reader_destroy(&bbr);
        return -1;
    }

    return 0;
}
/*
 * execute sync unary call
 */
static int tmc_sync_request(lua_State *L)
{
#ifdef DEBUG
  std::cout << "tmc_sync_request called" << std : endl;
#endif
  context *ctx = context_check(L, 1);
  const char *rpc_method = luaL_checkstring(L, 2);
  size_t msg_length = 0;
  const char *encoded_payload = luaL_checklstring(L, 3, &msg_length);
  // fixme:
  // workaround for throwing exception of gRPC core library APIs.
  try
  {
    struct gpr_timespec timeout = handle_optional_timeout_parameter(L, 4);

    struct grpc_completion_queue *queue = grpc_completion_queue_create_for_pluck(nullptr);
    struct grpc_slice method = grpc_slice_from_static_string(rpc_method);
    struct gpr_timespec deadline = create_timespec_infinite();

    // parent_call is NULL because this is grpc client request.
    grpc_call *call = grpc_channel_create_call(ctx->channel, nullptr, GRPC_PROPAGATE_DEFAULTS, queue, method, nullptr, timeout, nullptr);

    grpcwrap_batch_context *batch_context = grpcwrap_batch_context_create();

    struct grpc_slice payload = grpc_slice_from_static_buffer(encoded_payload, msg_length);

    // TODO: after supporting metadata feature, handling metadata is necessary.
    grpc_call_error error = grpcwrap_call_start_unary(call, batch_context, &payload, 0, 0, ctx);

    if (error != GRPC_CALL_OK)
    {
      cleanup(queue, batch_context);
      return luaL_error(L, "cannot start batch. grpc_call_error rc is %d\n", error);
    }

    // block until response arrived.
    grpc_event event = grpc_completion_queue_pluck(queue, ctx, deadline, nullptr);
    if (event.type != GRPC_OP_COMPLETE)
    {
      cleanup(queue, batch_context);
      return luaL_error(L, "cannot complete request");
    }

  #ifdef DEBUG
    std::cout << event.type << " " << event.success << std::endl;
    std::cout << "status code:" << batch_context->recv_status_on_client.status << std::endl;
  #endif

    int ret_count = 1;
    lua_pushinteger(L, batch_context->recv_status_on_client.status);
    // return grpc return code and receive status
    if (batch_context->recv_status_on_client.status == GRPC_STATUS_OK && batch_context->recv_message != nullptr)
    {
        // read from grpc message 
        grpc_slice res;
        int ret = internal_conv_buf2slice(batch_context->recv_message,&res);
        if (ret != 0) {
            return luaL_error(L, "unknown error occurred: %s\n", __func__);
        }

        // get msg size and push them.
        size_t msg_length = GRPC_SLICE_LENGTH(res);
        char *slice_data = reinterpret_cast<char *> GRPC_SLICE_START_PTR(res);
        lua_pushlstring(L, slice_data, msg_length);
        ret_count++;
    
        grpc_slice_unref(res);
    }

    // clean up
    cleanup(queue, batch_context);
    return ret_count;
  }
  catch(...)
  {
    return luaL_error(L,"unknown error occurred: %s\n",__func__);
  }

}

static int tmc_shutdown(lua_State *L)
{
  context *ctx = context_check(L, 1);

  // fixme:
  // workaround for throwing exception of gRPC core library APIs.
  try
  {
    if (ctx->channel)
    {
      grpc_channel_destroy(ctx->channel);
    }
    grpc_shutdown();
  }
  catch(...)
  {
    return luaL_error(L,"unknown error occurred: %s\n",__func__);
  }
  return 0;
}

static int tmc_cleanup(lua_State *L)
{
  (void)L;
  grpc_shutdown();
  return 0;
}

static void luaopen_grpc_stub_lib(lua_State *L)
{
  static const struct luaL_Reg grpcstub_lib[] = {
      {"new", tmc_new_grpc},
      {"__gc", tmc_cleanup},
      {NULL, NULL}};
  luaL_newlib(L, grpcstub_lib);
  return;
}

static const struct luaL_Reg context_methods[] = {
    {"sync_request", tmc_sync_request},
    {"shutdown", tmc_shutdown},
    {NULL, NULL}};

extern "C"
{
  int luaopen_tmc_wrap_grpc_stub(lua_State *L)
  {
    grpc_init();

    // create meta table.
    luaL_newmetatable(L, GRPC_STUB_META_CONTEXT);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, context_methods, 0);
    luaopen_grpc_stub_lib(L);

    return 1;
  }
}
