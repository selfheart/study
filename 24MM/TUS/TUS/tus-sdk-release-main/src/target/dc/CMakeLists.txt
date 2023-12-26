# TMC CONFIDENTIAL
# $TUSLibId$
# Copyright (C) 2022 TOYOTA MOTOR CORPORATION
# All Rights Reserved.

cmake_minimum_required(VERSION 3.13)

project(dc)

if ("${TUS_LIBCURL}" STREQUAL "module")
  # 'libcurl' is ${LIB_NAME} in src/lib/curl/lib/CMakeLists.txt
  set(CURL_LIBRARIES "libcurl")
else()
  find_package(CURL REQUIRED)
endif()
cmake_print_variables(CMAKE_CURRENT_LIST_FILE CURL_LIBRARIES)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if ("${TUS_LIBGRPC}" STREQUAL "module")
  set(GRPCXX_LIBRARIES "grpc++")
else()
  set(GRPCXX_LIBRARIES "-lgrpc++;-lprotobuf")
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_subdirectory(platform)  # for tus_dc_platform

add_executable(${PROJECT_NAME}
  fetch.c
  grpc_server.cpp
  main.c
  pal_util.c
  )

target_include_directories(${PROJECT_NAME} PRIVATE
  ${CURL_INCLUDE_DIR}
  ${tus_lua_SOURCE_DIR}
  )

target_link_libraries(${PROJECT_NAME}
  Threads::Threads
  tus_dc_platform
  tus_includes
  tus_lua_static
  tus_grpc_uodc
  tus_logger
  ${CURL_LIBRARIES}
  ${GRPCXX_LIBRARIES}
  )

set_property(TARGET ${PROJECT_NAME} PROPERTY ENABLE_EXPORTS 1)

# DC main & script env
file(INSTALL dc.lua ../common/async.lua ../common/config_util.lua
  ${luamodule_delta_update_SOURCE_DIR}/delta_update.lua
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(INSTALL sample/config
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
  USE_SOURCE_PERMISSIONS
  )

# updaters
file(INSTALL
  ./updater_debug.lua
  ./updater_dpkg.lua
  ./updater_file.lua
  ./updater_image.lua
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# debugging
file(INSTALL
  gen_script__delta.lua
  gen_script__download_url.lua
  gen_script__interactive.lua
  gen_script_ipkg1.lua
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/
  )

# note: DC_VERSION_FILES_DIR must not ends by '/', whech inhibis mkdir
set(DC_VERSION_FILES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/sample/version)

# * sample/version/domain is a dummy version file which contains "DOMAIN_VERSION_1"
file(INSTALL
  ${DC_VERSION_FILES_DIR}/domain
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/version/)

# * sample/version/vehicle_config shall contain {target-id, {{subtarget-id, sw-version}}}
#   that matchs SDPv3 ECU config
#   ex: for src/target/uo/sample/provider/sdpv3/ecu/01000 where initail states are
#   - ecu_software_part_a_id/3d500 = 1.0
#   - active_bank = A
#   , 'vehicle_config' shall contain '{ecu_target_id="01000", new_ecu_software_part_id={{subtarget_id="3d500", product_number="1.0",}}},'
file(INSTALL
  ${DC_VERSION_FILES_DIR}/vehicle_config
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/version/)


install(TARGETS dc
  DESTINATION ${INSTALL_PREFIX_DC} COMPONENT ${COMPONENT_DC})

install(FILES
  ${target_common_SOURCE_DIR}/async.lua
  ${target_common_SOURCE_DIR}/config_util.lua
  ./dc.lua
  ./updater_debug.lua
  ./updater_dpkg.lua
  ./updater_file.lua
  ./updater_image.lua
  DESTINATION ${INSTALL_PREFIX_DC} COMPONENT ${COMPONENT_DC})

install(DIRECTORY ${DC_VERSION_FILES_DIR}
  DESTINATION ${INSTALL_PREFIX_DC} COMPONENT ${COMPONENT_DC})
