# TMC CONFIDENTIAL
# $TUSLibId$
# Copyright (C) 2022 TOYOTA MOTOR CORPORATION
# All Rights Reserved.

cmake_minimum_required(VERSION 3.13)

project(tup_zlib)

add_library(${PROJECT_NAME}
  zlib.c
  )

set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE True)

target_include_directories(${PROJECT_NAME} PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  )

target_include_directories(${PROJECT_NAME} PRIVATE
  $<TARGET_PROPERTY:tup_parser,INCLUDE_DIRECTORIES>
  )

target_link_libraries(${PROJECT_NAME} PRIVATE
  tus_logger
  -lz
  )
