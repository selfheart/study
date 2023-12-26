# TMC CONFIDENTIAL
# $TUSLibId$
# Copyright (C) 2022 TOYOTA MOTOR CORPORATION
# All Rights Reserved.

cmake_minimum_required(VERSION 3.13)

project(tup_lz4)

if ("${TUS_LIBLZ4}" STREQUAL "module")
  set(LZ4_LIBRARIES lz4_static)
else()
  set(LZ4_LIBRARIES -llz4)
  find_path(LZ4_INCLUDE_DIR lz4.h HINTS "${CMAKE_SYSROOT}/usr/include/lz4")
endif()
cmake_print_variables(TUS_LIBLZ4 LZ4_INCLUDE_DIR LZ4_LIBRARIES)

add_library(${PROJECT_NAME}
  lz4.c
  )

set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE True)

target_include_directories(${PROJECT_NAME} PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  )

target_include_directories(${PROJECT_NAME} PRIVATE
  $<TARGET_PROPERTY:tup_parser,INCLUDE_DIRECTORIES>
  ${LZ4_INCLUDE_DIR}
  )

target_link_libraries(${PROJECT_NAME} PRIVATE
  tus_logger
  ${LZ4_LIBRARIES}
  )
