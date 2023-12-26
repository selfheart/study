# TMC CONFIDENTIAL
# $TUSLibId$
# Copyright (C) 2022 TOYOTA MOTOR CORPORATION
# All Rights Reserved.

cmake_minimum_required(VERSION 3.13)

# for add_lua_extension()
include(LuaWrapper)

if ("${TUS_LIBGRPC}" STREQUAL "module")
  set(GRPC_LIBS "grpc")
else()
  find_library(GRPC_LIBS grpc REQUIRED)
  cmake_print_variables(GRPC_LIBS)
  include(FindPkgConfig)
  pkg_check_modules(PC_GRPC grpc)
  cmake_print_variables(PC_GRPC_VERSION)
endif()

# tmc_wrap_grpc_stub
add_lua_extension(tmc_wrap_grpc_stub
  src/cpp/stub.cpp
  src/cpp/grpc_wrap_utility.cpp
  src/cpp/grpc_batch_context.cpp
  )
target_link_libraries(tmc_wrap_grpc_stub PRIVATE
  ${GRPC_LIBS}
  )
# fixme: workaround exceptions of gRPC core library
target_compile_options(tmc_wrap_grpc_stub
  PRIVATE -fexceptions
)

# tmc_wrap_grpc_sync_writer
add_lua_extension(tmc_wrap_grpc_sync_writer
  src/cpp/sync_writer.cpp
  src/cpp/grpc_wrap_utility.cpp
  src/cpp/grpc_batch_context.cpp
  )
target_link_libraries(tmc_wrap_grpc_sync_writer PRIVATE
  ${GRPC_LIBS}
  )
# fixme: workaround exceptions of gRPC core library
target_compile_options(tmc_wrap_grpc_sync_writer
  PRIVATE -fexceptions
)

# tmc_wrap_grpc_channel
add_lua_extension(tmc_wrap_grpc_channel
  src/cpp/channel.cpp
  )
target_link_libraries(tmc_wrap_grpc_channel PRIVATE
  ${GRPC_LIBS}
  )
# fixme: workaround exceptions of gRPC core library
target_compile_options(tmc_wrap_grpc_channel
  PRIVATE -fexceptions
)

if(PC_GRPC_VERSION MATCHES [[(^[0-9]+)]])
  if (${CMAKE_MATCH_1} LESS 23)
    # gRPC < 1.45, ecpect to have grpc_insecure_*()
  else()
    target_compile_definitions(tmc_wrap_grpc_channel
      PRIVATE GRPC_NO_INSECURE
      )
  endif()
endif()


install(FILES
  ./src/lua/tmc_grpc.lua
  ./src/lua/tmc_grpc_constant.lua
  DESTINATION ${INSTALL_PREFIX_UO} COMPONENT ${COMPONENT_UO})

install(TARGETS
  tmc_wrap_grpc_channel
  tmc_wrap_grpc_stub
  tmc_wrap_grpc_sync_writer
  DESTINATION ${INSTALL_PREFIX_UO} COMPONENT ${COMPONENT_UO})
