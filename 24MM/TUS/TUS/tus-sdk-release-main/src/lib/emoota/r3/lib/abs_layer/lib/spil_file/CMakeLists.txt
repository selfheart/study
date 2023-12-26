# -*- mode: cmake; -*-
cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
# File: spil_file/CMakeLists.txt
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

project(${EMOOTA3}spil_file)

add_library(${PROJECT_NAME} STATIC)
set_property(TARGET ${EMOOTA3}spil_file PROPERTY POSITION_INDEPENDENT_CODE TRUE)

#Non-force allowing project-external definition.)
set(OTAMATIC_PLAT_OS posix CACHE STRING  "one of: posix|freertos" )
set(PLAT_SRC)     #Platform specific source files.
set(PLAT_LIB)     #Platform specific library linkage.
set(SOURCE_FILES) #Shared source files.

option(BUILD_${PROJECT_NAME}_TESTS "Build test suite for ${PROJECT_NAME}." OFF)
if(BUILD_${PROJECT_NAME}_TESTS AND WITH_COVERAGE)
  add_compile_options(-g O0 -fprofile-arcs -ftest-coverage)
endif()
if (DEFINED INSTALL_BASE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE INSTALL_BASE="${INSTALL_BASE}")
endif ()

get_filename_component(CWD ${PROJECT_SOURCE_DIR} REALPATH)
if (${OTAMATIC_PLAT_OS} MATCHES "freertos")
    set(PLAT_SRC ${CWD}/src/spil_file_fatfs.c )
    set(PLAT_LIBS freertos )
else()
    # default as POSIX Linux
    set(OTAMATIC_PLAT_OS posix)  #does this need pushed upwards to
                                 #parent scope?
    set(PLAT_SRC ${CWD}/src/spil_file_posix.c )
endif()

set(SOURCE_FILES ${CWD}/src/abq_files.c
                 ${CWD}/src/spil_file_common.c
                 ${CWD}/src/spil_file_tar.c
                 ${CWD}/src/ontrac/files.c )
list(APPEND SOURCE_FILES ${PLAT_SRC})


if(BUILD_${PROJECT_NAME}_TESTS)
	get_filename_component(TEST "${CWD}/test" REALPATH)
        #Why is this not a source input to the test target?
	#list(APPEND SOURCE_FILES ${TEST}/src/spil_file/spil_file_test.c )
endif()

target_sources(${PROJECT_NAME} PRIVATE ${SOURCE_FILES})

target_include_directories(${PROJECT_NAME}
  PUBLIC ${CWD}/include
)
target_link_libraries(${PROJECT_NAME}
  PRIVATE ${EMOOTA3}platform ${EMOOTA3}ontrac ${EMOOTA3}spil_os ${PLAT_LIBS} )

if(BUILD_${PROJECT_NAME}_TESTS)
  include(CTest)
  enable_testing()

  add_subdirectory(test)
  include(AirbiquityUtils)
  # Fetch aqmock ONLY IF spil_file is top level of the current CMake source tree.
  if(${CMAKE_SOURCE_DIR} STREQUAL ${PROJECT_SOURCE_DIR})
      if(NOT TARGET aqmock)
          import_sdm_c_module("aqmock" "integration")
      endif ()
  endif ()
else ()
    include(AirbiquityUtils)
endif ()

# function is defined in AirbiquityUtils.cmake
define_filename_macro_for_sources(${PROJECT_NAME})

# directory not available for customer deliveries
if (EXISTS doc)
    add_subdirectory(doc)
endif ()
