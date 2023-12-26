# TMC CONFIDENTIAL
# $TUSLibId$
# Copyright (C) 2022 TOYOTA MOTOR CORPORATION
# All Rights Reserved.

cmake_minimum_required(VERSION 3.13)

project(tus)

include(CMakeDependentOption)
include(CMakePrintHelpers)

if (CMAKE_CROSSCOMPILING)
  cmake_print_variables(CMAKE_TOOLCHAIN_FILE)
  cmake_print_variables(CMAKE_DL_LIBS)
endif()

set(TUS_REPO_ROOT ${CMAKE_CURRENT_LIST_DIR})
cmake_print_variables(TUS_REPO_ROOT)

# default compiler flags
set(COMPILER_FLAGS "-g -Wall -Wextra -Wvla -Werror=format -Werror=vla -O2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMPILER_FLAGS} -fno-rtti -fno-exceptions")
set(CMAKE_C_FLAGS "-std=c11 -D_POSIX_C_SOURCE=200809L ${CMAKE_C_FLAGS} ${COMPILER_FLAGS}")

# prefer static library to reduce number of files to distribute
# (most of custom libraries are not to be shared with others anyway).
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

# for build type 'Coverage', to use LLVM_PROFILE_FILE="%p.profraw"
# Ex.) cmake -DCMAKE_BUILD_TYPE=Coverage /path/to/jitdf/
# note: requires to build using clang & clang++
# note: '-O0' here is expected to override '-O2' above
set(CMAKE_C_FLAGS_COVERAGE "-fprofile-instr-generate -fcoverage-mapping -g -O0")
set(CMAKE_CXX_FLAGS_COVERAGE "-fprofile-instr-generate -fcoverage-mapping -g -O0")
set(CMAKE_EXE_LINKER_FLAGS_COVERAGE "-fprofile-instr-generate -fcoverage-mapping")
set(CMAKE_SHARED_LINKER_FLAGS_COVERAGE "-fprofile-instr-generate -fcoverage-mapping")


# enable Clang Tidy if 'CLANG_TIDY_CHECKS' was given
option(TUS_CLANG_TIDY "enable clang-tidy" OFF)
if(TUS_CLANG_TIDY)
  set(CMAKE_C_CLANG_TIDY "clang-tidy")
  set(CMAKE_CXX_CLANG_TIDY "clang-tidy")
  cmake_print_variables(CMAKE_C_CLANG_TIDY CMAKE_CXX_CLANG_TIDY)
endif()

# add 'test' as a target
# - shall run the "install" target before testing
# - use "ctest -V" or "make test ARGS='-V'" to see all outputs
# - run 'ctest -N' to get list of defined tests
# - run 'ctest -R <regex>' to select tests to run by a regexp
# - run 'ctest -T <action>' to select action for CDash
#   e.g. 'ctest -T memcheck' runs all unittest with memory check command
include(CTest)
enable_testing()

## UO shall be able to run using files under ${INSTALL_PREFIX_UO} (+ configs)
set(INSTALL_PREFIX_UO "deploy/target/uo")

set(COMPONENT_UO "target_uo")
# to copy UO config files, needed to run uo from ${INSTALL_PREFIX_UO}
# note: expect 'EXCLUDE_FROM_ALL', to be installed explicitly if needed as
#  cmake --install ... --component target_uo_sample
set(COMPONENT_UO_SAMPLE "target_uo_sample")

## DC shall be able to run using files under ${INSTALL_PREFIX_DC}
set(INSTALL_PREFIX_DC "deploy/target/dc")
set(COMPONENT_DC "target_dc")

if (CMAKE_CROSSCOMPILING)
  message(WARNING "SKIPPING document generation for cross-compile")
else()
endif()

if (EXISTS ${CMAKE_CURRENT_LIST_DIR}/src/lib/emoota/r3/CMakeLists.txt)
  set(TUS_UO_PROVIDER_SDPV3 ON CACHE BOOL "enable SDPv3 provider")
else()
  set(TUS_UO_PROVIDER_SDPV3 OFF CACHE BOOL "enable SDPv3 provider")
endif()
cmake_print_variables(TUS_UO_PROVIDER_SDPV3)


# 'package': use system libs, 'module': build from this repository
set(TUS_LIBCURL "module" CACHE STRING "Provider of cURL library")
set_property(CACHE TUS_LIBCURL PROPERTY STRINGS "module" "package")
cmake_print_variables(TUS_LIBCURL)

set(TUS_LIBGRPC "package" CACHE STRING "Provider of gRPC library")
set_property(CACHE TUS_LIBGRPC PROPERTY STRINGS "module" "package")
cmake_print_variables(TUS_LIBGRPC)

set(TUS_LIBWEBSOCKETS "package" CACHE STRING "Provider of libwebsockets")
set_property(CACHE TUS_LIBWEBSOCKETS PROPERTY STRINGS "module" "package")
cmake_print_variables(TUS_LIBWEBSOCKETS)

set(TUS_LIBLZ4 "package" CACHE STRING "Provider of liblz4")
set_property(CACHE TUS_LIBLZ4 PROPERTY STRINGS "module" "package")
cmake_print_variables(TUS_LIBLZ4)

set(TUS_CRYPT_LIBRARY "openssl" CACHE STRING "Provider of CRYPT library")
set_property(CACHE TUS_CRYPT_LIBRARY PROPERTY STRINGS "mbedtls" "openssl")
cmake_print_variables(TUS_CRYPT_LIBRARY)

if ("${TUS_CRYPT_LIBRARY}" STREQUAL "mbedtls")
  set(TUS_LIBMBEDTLS "package" CACHE STRING "Provider of MbedTLS library")
  set_property(CACHE TUS_LIBMBEDTLS PROPERTY STRINGS "module" "package")
  cmake_print_variables(TUS_LIBMBEDTLS)
endif()

add_subdirectory(error_code)
add_subdirectory(src)

add_library(tus_includes INTERFACE)
target_include_directories(tus_includes INTERFACE ./include)

# let './' be the target of 'install'
set(CMAKE_INSTALL_PREFIX "./" CACHE PATH "default packaging prefix" FORCE)
