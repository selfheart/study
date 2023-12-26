# TMC CONFIDENTIAL
# $TUSLibId$
# Copyright (C) 2022 TOYOTA MOTOR CORPORATION
# All Rights Reserved.

cmake_minimum_required(VERSION 3.13)

project(lz4)

# liblz4.so, to be linked as 'lz4'
add_library(${PROJECT_NAME} SHARED
  lib/lz4.c
  lib/lz4frame.c
  lib/lz4hc.c
  lib/xxhash.c
  )
target_include_directories(${PROJECT_NAME} PUBLIC
  lib/
  )
#set_target_properties(${PROJECT_NAME} PROPERTIES
#  SOVERSION "9"
#  VERSION "1.9.3")

# also build liblz4_static.a, to be linked as 'lz4_static'
add_library(${PROJECT_NAME}_static STATIC
  $<TARGET_PROPERTY:${PROJECT_NAME},SOURCES>
  )
target_include_directories(${PROJECT_NAME}_static PUBLIC
  lib/
  )
set_target_properties(${PROJECT_NAME} PROPERTIES
  POSITION_INDEPENDENT_CODE ON)

# ignore 'lz4' executable for now
