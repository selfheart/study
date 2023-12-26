# TMC CONFIDENTIAL
# $TUSLibId$
# Copyright (C) 2022 TOYOTA MOTOR CORPORATION
# All Rights Reserved.

cmake_minimum_required(VERSION 3.13)
project(emoota3)
set(EMOOTA3 "${PROJECT_NAME}_")
#include(CPackConfig.cmake)

include(CMakePrintHelpers)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
cmake_print_variables(CMAKE_MODULE_PATH)

include(ConfigEMoota) # put project config options here.
include(ConfigCCache) # enable CCache for fast builds with EMOOTA_CCACHE_BUILD=ON.
                      # above could be consolidated into ConfigEMoota.cmake

option(TESTRUNNER "Use this option to enable cmocka asserts in targets other than the testrunner" OFF)
set(INSTALL_BASE /data CACHE PATH "base config/data dir like /data")
#set(INSTALL_BASE ${CMAKE_INSTALL_LOCALSTATEDIR})   # requires proper permissions

add_subdirectory(lib/utility)
add_subdirectory(lib/abs_layer)
add_subdirectory(lib/uptane)
set_property(TARGET ${EMOOTA3}utility PROPERTY POSITION_INDEPENDENT_CODE TRUE)
set_property(TARGET ${EMOOTA3}abs_layer PROPERTY POSITION_INDEPENDENT_CODE TRUE)
set_property(TARGET ${EMOOTA3}uptane PROPERTY POSITION_INDEPENDENT_CODE TRUE)

if(DEFINED INSTALL_BASE)
    target_compile_definitions(${EMOOTA3}abs_layer PUBLIC INSTALL_BASE="${INSTALL_BASE}")
endif()

