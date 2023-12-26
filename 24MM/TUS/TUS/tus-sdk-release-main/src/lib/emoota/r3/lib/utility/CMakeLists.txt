# TMC CONFIDENTIAL
# $TUSLibId$
# Copyright (C) 2022 TOYOTA MOTOR CORPORATION
# All Rights Reserved.

cmake_minimum_required(VERSION 3.13)
project(${EMOOTA3}utility C)

set( CMAKE_C_STANDARD 11 )
set( CMAKE_C_STANDARD_REQUIRED ON )
set( CMAKE_C_EXTENSIONS OFF )


list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

#Sanity check, defend against in-source-tree builds.
if( CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR AND NOT MSVC_IDE )
  message(FATAL_ERROR "In-source builds are not allowed.
Please create a directory and run cmake from there, passing the path
to this source directory as the last argument.
This process created the file `CMakeCache.txt' and the directory `CMakeFiles'.
Please delete them.")
endif()

option(EMOOTA_INCLUDE_TESTS "Generate build targets for EMOOTA tests." OFF)
option(EMOOTA_BUILD_TESTS   "Build EMOOTA unit tests. If OFF, just generate build targets." OFF)
option(EMOOTA_TEST_RECORDS  "Record test execution results." OFF)
option(EMOOTA_TEST_COVERAGE "Measure test coverage." OFF)
set(EMOOTA_TEST_RECORD_FORMAT "XML"
  CACHE STRING
  "One of 'NONE';'XML'. This setting is ignored when either          \
  `EMOOTA_TEST_RECORDS` or `EMOOTA_BUILD_TESTS` is disabled. Specifies the \
   format of test execution records") #force?
include(EMOOTATestConfig)

add_library(${PROJECT_NAME} STATIC)
set(utility_SOURCES
        src/emoota_utility.c
        src/ascii.c
        src/itsy_bits.c
        src/codec.c
        src/utf8_enc.c
        src/hex_enc.c
        src/string_db.c   )

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}> )
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_sources(${PROJECT_NAME} PRIVATE ${utility_SOURCES})
target_link_libraries(${PROJECT_NAME} PUBLIC tiny_json)

if (${CMAKE_BUILD_TYPE} MATCHES Debug)
  target_compile_definitions(${PROJECT_NAME} PUBLIC DEBUG_BUILD)
endif ()
if (${TESTRUNNER})
  target_compile_definitions(${PROJECT_NAME} PUBLIC TESTRUNNER)
endif ()


include(CMakePrintHelpers)
cmake_print_properties(TARGETS ${PROJECT_NAME} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES)

#Test definitions below:
if(EMOOTA_INCLUDE_TESTS)
  include(CTest)
  enable_testing()
  add_subdirectory(test)

  # I wish there was a cmake-generator-expression(7) to obtain global
  # property.  Then this test block could be defined lexically anywhere
  # in the file, not after all subdirs have been added.
  get_property(emoota_testsuite GLOBAL PROPERTY EMOOTA_TESTSUITE)
  get_property(emoota_testsuite_runners GLOBAL PROPERTY EMOOTA_TESTSUITE_RUNNERS)

  add_custom_target(testsuite
    COMMENT "Building full EMOOTA test suite."
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${emoota_testsuite} )

  add_custom_target(check-all
    COMMENT "Running full OTA test suite."
    COMMAND ${CMAKE_CTEST_COMMAND} -VV
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${emoota_testsuite}  )

endif()

