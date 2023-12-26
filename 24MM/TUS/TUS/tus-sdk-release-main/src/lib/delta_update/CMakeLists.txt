# TMC CONFIDENTIAL
# $TUSLibId$
# Copyright (C) 2022 TOYOTA MOTOR CORPORATION
# All Rights Reserved.

cmake_minimum_required(VERSION 3.13)

project(delta_update)

add_library(${PROJECT_NAME} STATIC
  src/delta_update.c
  src/delta_update_bsdiff.c
  )

set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE True)

target_include_directories(${PROJECT_NAME} PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  )

target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  )

target_link_libraries(${PROJECT_NAME} PRIVATE
  tus_logger
  )
