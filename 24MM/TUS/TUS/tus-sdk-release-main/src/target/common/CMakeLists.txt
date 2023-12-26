# TMC CONFIDENTIAL
# $TUSLibId$
# Copyright (C) 2022 TOYOTA MOTOR CORPORATION
# All Rights Reserved.

cmake_minimum_required(VERSION 3.13)

project(target_common)

if ("${TUS_LIBGRPC}" STREQUAL "module")
  # use headers in this repository, instead of system's
  set(Protobuf_INCLUDE_DIR "${protobuf_SOURCE_DIR}/../src;${grpc_SOURCE_DIR}/include")
  cmake_print_variables(Protobuf_INCLUDE_DIR)
  option(TUS_LIBGRPC_USE_PREBUILT "use pre-generated gRPC sources" ON)
else()
  option(TUS_LIBGRPC_USE_PREBUILT "use pre-generated gRPC sources" OFF)
endif()

find_program(_PROTOBUF_PROTOC protoc)
cmake_print_variables(_PROTOBUF_PROTOC)


find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
cmake_print_variables(_GRPC_CPP_PLUGIN_EXECUTABLE)


# UO-DC gRPC

get_filename_component(dc_proto "./dc.proto" ABSOLUTE)
get_filename_component(dc_proto_dir "${dc_proto}" PATH)
cmake_print_variables(dc_proto)
cmake_print_variables(dc_proto_dir)


## for Lua

set(dc_luamod "${CMAKE_CURRENT_BINARY_DIR}/dc_pb.lua")
set(dc_grpc_luamod "${CMAKE_CURRENT_BINARY_DIR}/dc_pb_grpc.lua")

set(PROTC_LUA_BASE ${TUS_REPO_ROOT}/src/tools/protoc)

if (TUS_LIBGRPC_USE_PREBUILT)
  add_custom_command(
    OUTPUT "${dc_luamod}" "${dc_grpc_luamod}"
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/dc_pb.lua
    ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/dc_pb_grpc.lua
    ${CMAKE_CURRENT_BINARY_DIR}
    )
else()
  # ToDo: may better to copy plugin;s *.py into builddir to avoid creating "__pycache__"s
  add_custom_command(
    OUTPUT "${dc_luamod}" "${dc_grpc_luamod}"
    COMMAND ${_PROTOBUF_PROTOC}
    ARGS
      --grpc_out "lite:${CMAKE_CURRENT_BINARY_DIR}"
      --lua_out "lite:${CMAKE_CURRENT_BINARY_DIR}"
      -I "${dc_proto_dir}"
      --plugin=${PROTC_LUA_BASE}/protoc_lua_plugin/protoc-gen-lua
      --plugin=protoc-gen-grpc=${PROTC_LUA_BASE}/protoc_grpc_lua_plugin/protoc-gen-grpc_lua
      "${dc_proto}"
    DEPENDS "${dc_proto}")
endif()
add_custom_target(uodc_lua ALL
  DEPENDS  "${dc_luamod}" "${dc_grpc_luamod}"
  )

## for C++

set(dc_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/dc.pb.cc")
set(dc_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/dc.pb.h")

set(dc_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/dc.grpc.pb.cc")
set(dc_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/dc.grpc.pb.h")

if (TUS_LIBGRPC_USE_PREBUILT)
  add_custom_command(
    OUTPUT "${dc_proto_srcs}" "${dc_proto_hdrs}"
      "${dc_grpc_srcs}" "${dc_grpc_hdrs}"
      COMMAND ${CMAKE_COMMAND} -E copy
      ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/dc.pb.cc
      ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/dc.pb.h
      ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/dc.grpc.pb.cc
      ${CMAKE_CURRENT_SOURCE_DIR}/prebuilt/dc.grpc.pb.h
      ${CMAKE_CURRENT_BINARY_DIR}
    )
else()
  add_custom_command(
    OUTPUT "${dc_proto_srcs}" "${dc_proto_hdrs}"
      "${dc_grpc_srcs}" "${dc_grpc_hdrs}"
    COMMAND ${_PROTOBUF_PROTOC}
    ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
      --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
      -I "${dc_proto_dir}"
      --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
      "${dc_proto}"
    DEPENDS "${dc_proto}")
endif()

add_library(tus_grpc_uodc STATIC
  ${dc_proto_hdrs}
  ${dc_proto_srcs}
  ${dc_grpc_hdrs}
  ${dc_grpc_srcs}
  )

target_include_directories(tus_grpc_uodc PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}
  ${Protobuf_INCLUDE_DIR}
  )
target_compile_definitions(tus_grpc_uodc PUBLIC
  GOOGLE_PROTOBUF_NO_RTTI
  )

# logger
find_path(C11_THREADS threads.h)
cmake_print_variables(CMAKE_CURRENT_LIST_FILE C11_THREADS)
add_library(tus_logger STATIC
  logger.c
  logger.h
  )
target_include_directories(tus_logger PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  )
if (${C11_THREADS} STREQUAL "C11_THREADS-NOTFOUND")
  message(WARNING "C11 thread is unavailable")
  target_compile_definitions(tus_logger PRIVATE MISSING_C11_THREADS)
endif()
set_property(TARGET tus_logger PROPERTY POSITION_INDEPENDENT_CODE TRUE)


# Lua extensions
add_subdirectory(ext)


install(FILES "${dc_luamod}" "${dc_grpc_luamod}"
  DESTINATION ${INSTALL_PREFIX_UO} COMPONENT ${COMPONENT_UO})
