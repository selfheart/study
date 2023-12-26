# TMC CONFIDENTIAL
# $TUSLibId$
# Copyright (C) 2022 TOYOTA MOTOR CORPORATION
# All Rights Reserved.

cmake_minimum_required(VERSION 3.13)

# for add_lua_extension()
include(LuaWrapper)

if ("${TUS_LIBGRPC}" STREQUAL "module")
  set(Protobuf_LITE_LIBRARIES "libprotobuf-lite")
else()
  # note: refrain from using find_package(Protobuf REQUIRED)
  # to ignore host's 'protoc' while cross-compiling
  find_path(Protobuf_INCLUDE_DIR
    google/protobuf/service.h
    )
  find_library(Protobuf_LITE_LIBRARIES
    protobuf-lite
    )
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_lua_extension(tmc_protobuf
  protobuf_char_wrapper.cpp
  protobuf_eps_copy_output_stream_wrapper.cpp
  protobuf_helper.cpp
  protobuf_internal_metadata_wrapper.cpp
  protobuf_internal_wrapper.cpp
  protobuf_message_lite_wrapper.cpp
  protobuf_message_lite.cpp
  protobuf_parse_context_wrapper.cpp
  protobuf_wire_format_lite_wrapper.cpp
  protobuf_wrapper.cpp
  )
target_compile_options(tmc_protobuf
  PRIVATE -fexceptions
  )
target_compile_definitions(tmc_protobuf
  PRIVATE GOOGLE_PROTOBUF_NO_RTTI
  )
target_include_directories(tmc_protobuf PRIVATE
  ${Protobuf_INCLUDE_DIR}
  )
target_link_libraries(tmc_protobuf PRIVATE
  Threads::Threads
  ${Protobuf_LITE_LIBRARIES}
  )

install(FILES
  ./tmc_protobuf_helper.lua
  ./tmc_protobuf_repeated_message_field.lua
  ./tmc_protobuf_map_field.lua
  ./tmc_protobuf_repeated_value_field.lua
  ./tmc_protobuf_oneof_field.lua
  ./tmc_protobuf_message_lite.lua
  DESTINATION ${INSTALL_PREFIX_UO} COMPONENT ${COMPONENT_UO})

install(TARGETS tmc_protobuf
  DESTINATION ${INSTALL_PREFIX_UO} COMPONENT ${COMPONENT_UO})
