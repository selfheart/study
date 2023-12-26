# -*- mode: cmake; -*-
cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
#[=======================================================================[.rst:

          AIRBIQUITY INC. PROPRIETARY AND CONFIDENTIAL INFORMATION

   The software and information contained herein is the proprietary and
   confidential property of Airbiquity Inc. and shall not be disclosed
   in whole or in part. Possession, use, reproduction or transfer of
   this software and information is prohibited without the express written
   permission of Airbiquity Inc.

   Copyright © 2021 Airbiquity Inc.  All rights reserved.

#]=======================================================================]#

# TMC CONFIDENTIAL
# $TUSLibId$
# Copyright (C) 2022 TOYOTA MOTOR CORPORATION
# All Rights Reserved.

project(${EMOOTA3}uptane)

set_directory_properties(
  PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS NO )

add_library(${PROJECT_NAME} STATIC)

set(SRCDIR ${PROJECT_SOURCE_DIR}/src)

set(${PROJECT_NAME}_SOURCES
    ${SRCDIR}/uptane.c
    ${SRCDIR}/metadata_sig.c
    ${SRCDIR}/meta_cached.c
    ${SRCDIR}/metadata.c
    ${SRCDIR}/manifest.c
    ${SRCDIR}/full_verification.c
)

target_sources(${PROJECT_NAME} PRIVATE ${${PROJECT_NAME}_SOURCES})

target_include_directories(${PROJECT_NAME}
  PUBLIC ${PROJECT_SOURCE_DIR}/include
  PUBLIC ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE ${EMOOTA3}utility ${EMOOTA3}abs_layer)

if(${TESTRUNNER})
  target_link_libraries(${PROJECT_NAME} PRIVATE  cmocka)
  target_compile_definitions(${PROJECT_NAME} PUBLIC TESTRUNNER)
endif()


# function is defined in AirbiquityUtils.cmake
define_filename_macro_for_sources(${PROJECT_NAME})
