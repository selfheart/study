# TMC CONFIDENTIAL
# $TUSLibId$
# Copyright (C) 2022 TOYOTA MOTOR CORPORATION
# All Rights Reserved.

cmake_minimum_required(VERSION 3.13)

project(tus_dc_platform)

# ToDo: operation strategy?
set(TUS_DC_PLATFORM_LINK_TYPE "STATIC" CACHE STRING "how PAL shall be built")
set_property(CACHE TUS_DC_PLATFORM_LINK_TYPE PROPERTY STRINGS "STATIC" "SHARED")

set(TUS_DC_PLATFORM "example" CACHE STRING "select platfrom-depenent implementatin")
set_property(CACHE TUS_DC_PLATFORM PROPERTY STRINGS "example")

add_library(${PROJECT_NAME} ${TUS_DC_PLATFORM_LINK_TYPE}
  ${TUS_DC_PLATFORM}.c
  )
set_target_properties(${PROJECT_NAME} PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/${TUS_DC_PLATFORM})

# ToDo: set SONAME / LIBRARY_VERSION ?
