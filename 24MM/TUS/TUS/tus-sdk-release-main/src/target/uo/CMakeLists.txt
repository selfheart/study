# TMC CONFIDENTIAL
# $TUSLibId$
# Copyright (C) 2022 TOYOTA MOTOR CORPORATION
# All Rights Reserved.

cmake_minimum_required(VERSION 3.13)


if ("${TUS_LIBGRPC}" STREQUAL "module")
  set(GRPCXX_LIBRARIES "grpc++")
else()
  set(GRPCXX_LIBRARIES "-lgrpc++;-lprotobuf")
endif()


set(TUS_UO_PROVIDER_LIST)

if (TUS_UO_PROVIDER_SDPV3)
  add_subdirectory(ext/emoota/r3)
  list(APPEND TUS_UO_PROVIDER_LIST sdpv3)
  list(APPEND TUS_UO_PROVIDER_LIST ${CMAKE_CURRENT_BINARY_DIR}/libsdpv3.so)
endif()

add_custom_target(uo_grpc ALL
  DEPENDS
  runner
  ${CMAKE_CURRENT_BINARY_DIR}/dc_pb_grpc.lua
  ${CMAKE_CURRENT_BINARY_DIR}/dc_pb.lua
  ${CMAKE_CURRENT_BINARY_DIR}/tmc_protobuf.so
  ${CMAKE_CURRENT_BINARY_DIR}/tmc_grpc.lua
  ${CMAKE_CURRENT_BINARY_DIR}/tmc_grpc_constant.lua
  ${CMAKE_CURRENT_BINARY_DIR}/tmc_wrap_grpc_channel.so
  ${CMAKE_CURRENT_BINARY_DIR}/tmc_wrap_grpc_stub.so
  ${CMAKE_CURRENT_BINARY_DIR}/tmc_wrap_grpc_sync_writer.so
  ${CMAKE_CURRENT_BINARY_DIR}/tmc_protobuf_helper.lua
  ${CMAKE_CURRENT_BINARY_DIR}/tmc_protobuf_repeated_message_field.lua
  ${CMAKE_CURRENT_BINARY_DIR}/tmc_protobuf_map_field.lua
  ${CMAKE_CURRENT_BINARY_DIR}/tmc_protobuf_repeated_value_field.lua
  ${CMAKE_CURRENT_BINARY_DIR}/tmc_protobuf_oneof_field.lua
  ${CMAKE_CURRENT_BINARY_DIR}/tmc_protobuf_message_lite.lua
  ${TUS_UO_PROVIDER_LIST}
  )

file(INSTALL
  main.lua ota_client.lua uo.lua uo_fsm.lua
  ../common/async.lua ../common/config_util.lua
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
  USE_SOURCE_PERMISSIONS
  )

file(INSTALL
  ext/provider
  gen_script.lua
  gen_script__delta.lua
  gen_script__interactive.lua
  gen_script__withurl.lua
  sample/config
  sample/domain
  sample/provider
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
  USE_SOURCE_PERMISSIONS
  )

# runtime_depend(relative_directory [list of names])
function (runtime_dependancy RELDIR)
  foreach(NAME IN LISTS ARGN)
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${NAME}
      COMMAND ${CMAKE_COMMAND} -E copy ${RELDIR}/${NAME} ${CMAKE_CURRENT_BINARY_DIR}/${NAME}
      DEPENDS ${RELDIR}/${NAME})
  endforeach()
endFunction()

runtime_dependancy(${target_common_BINARY_DIR}
  dc_pb.lua dc_pb_grpc.lua)

runtime_dependancy(${target_common_BINARY_DIR}/ext/protobuf/
  tmc_protobuf.so
  )
runtime_dependancy(${target_common_SOURCE_DIR}/ext/protobuf/
  tmc_protobuf_helper.lua
  tmc_protobuf_repeated_message_field.lua
  tmc_protobuf_map_field.lua
  tmc_protobuf_repeated_value_field.lua
  tmc_protobuf_oneof_field.lua
  tmc_protobuf_message_lite.lua
  )

runtime_dependancy(${target_common_BINARY_DIR}/ext/grpc/
  tmc_wrap_grpc_channel.so
  tmc_wrap_grpc_stub.so
  tmc_wrap_grpc_sync_writer.so
  )
runtime_dependancy(${target_common_SOURCE_DIR}/ext/grpc/src/lua/
  tmc_grpc.lua
  tmc_grpc_constant.lua
  )

runtime_dependancy(${CMAKE_CURRENT_BINARY_DIR}/ext/emoota/r3/
  libsdpv3.so
  )

find_library(LIBSOCKET socket)
if (${LIBSOCKET} STREQUAL "LIBSOCKET-NOTFOUND")
  set(LIBSOCKET "")
endif()

# TUS UO main
add_executable(uo
  main.c
  worker/common.c
  )
target_include_directories(uo PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${tus_lua_SOURCE_DIR}
  )
target_link_libraries(uo
  tus_includes
  tus_logger
  tus_lua_static
  ${LIBSOCKET}
  )
set_property(TARGET uo PROPERTY ENABLE_EXPORTS 1)

if ("${TUS_LIBWEBSOCKETS}" STREQUAL "package")
  # use system's libraries
  find_library(LIBWEBSOCKETS websockets)
  cmake_print_variables(LIBWEBSOCKETS)
  if (${LIBWEBSOCKETS} STREQUAL "LIBWEBSOCKETS-NOTFOUND")
    message(FATAL_ERROR "missing libwebsockets")
  endif()
elseif("${TUS_LIBWEBSOCKETS}" STREQUAL "module")
  # built from TUS repo.
  set(LIBWEBSOCKETS websockets)
else()
  cmake_print_variables(TUS_LIBWEBSOCKETS)
  message(FATAL_ERROR "invalid TUS_LIBWEBSOCKETS")
endif()

target_sources(uo PRIVATE worker/httpd.c)
target_link_libraries(uo ${LIBWEBSOCKETS} tup_parser)
target_compile_definitions(uo PRIVATE HAS_LIBWEBSOCKETS)

# TUP UO script runtime
add_executable(runner
  runner.c
  runner_dc_grpc.cpp
  )

target_include_directories(runner PRIVATE
  # Include generated *.pb.h files
  ${CMAKE_CURRENT_BINARY_DIR}
  ${tus_lua_SOURCE_DIR}
  )

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if ("${TUS_LIBCURL}" STREQUAL "module")
  # 'libcurl' is ${LIB_NAME} in src/lib/curl/lib/CMakeLists.txt
  set(CURL_LIBRARIES "libcurl")
else()
  find_package(CURL REQUIRED)
endif()
cmake_print_variables(CMAKE_CURRENT_LIST_FILE CURL_LIBRARIES)

target_link_libraries(runner
  Threads::Threads
  tus_includes
  tus_grpc_uodc
  tus_logger
  tus_lua_static
  ${CURL_LIBRARIES}
  ${GRPCXX_LIBRARIES}
  )

set_property(TARGET runner PROPERTY ENABLE_EXPORTS 1)

install(TARGETS runner uo
  DESTINATION ${INSTALL_PREFIX_UO} COMPONENT ${COMPONENT_UO})

install(FILES
  ${target_common_SOURCE_DIR}/async.lua
  ${target_common_SOURCE_DIR}/config_util.lua
  ./main.lua
  ./uo.lua
  ./uo_fsm.lua
  DESTINATION ${INSTALL_PREFIX_UO} COMPONENT ${COMPONENT_UO})

# place ext/provider/*.lua as provider/*.lua
install(DIRECTORY
  ./ext/provider
  DESTINATION ${INSTALL_PREFIX_UO} COMPONENT ${COMPONENT_UO})

# note: trailing '/' prevents creating 'sample/'
install(DIRECTORY ./sample/
  DESTINATION ${INSTALL_PREFIX_UO} COMPONENT ${COMPONENT_UO_SAMPLE} EXCLUDE_FROM_ALL)
