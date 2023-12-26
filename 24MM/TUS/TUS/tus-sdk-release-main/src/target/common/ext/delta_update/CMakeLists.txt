# TMC CONFIDENTIAL
# $TUSLibId$
# Copyright (C) 2022 TOYOTA MOTOR CORPORATION
# All Rights Reserved.

cmake_minimum_required(VERSION 3.13)

project(luamodule_delta_update)

# for add_lua_extension()
include(LuaWrapper)

add_lua_extension(tmc_delta_update
  module.c
  )

target_link_libraries(tmc_delta_update
  delta_update
  tus_logger
  )

# copy tmc_delta_update.so so that a dc run from a builddir may load it
# note: delta_update.lua shall be copyed by DC/s CMakeLists.txt using file(INSTALL ...)
#       since it shall be copied if edited, and file() does not accept generator expr.
add_custom_command(TARGET tmc_delta_update
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:tmc_delta_update> $<TARGET_FILE_DIR:dc>
  )
file(INSTALL
  ./delta_update.lua
  ./test.lua
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# populate deploy/target/dc for 'cmake --install'
# (note that resulting 'tmc_delta_update.so' will be stripped)
install(TARGETS
  tmc_delta_update
  DESTINATION ${INSTALL_PREFIX_DC} COMPONENT ${COMPONENT_DC}
  )
install(FILES
  delta_update.lua
  DESTINATION ${INSTALL_PREFIX_DC} COMPONENT ${COMPONENT_DC}
  )
