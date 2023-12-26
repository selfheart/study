# TMC CONFIDENTIAL
# $TUSLibId$
# Copyright (C) 2022 TOYOTA MOTOR CORPORATION
# All Rights Reserved.

cmake_minimum_required(VERSION 3.13)
project(${EMOOTA3}abs_layer C)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/lib/cmake_modules")
include(AirbiquityUtils)


if ("${TUS_LIBCURL}" STREQUAL "module")
  # 'libcurl' is ${LIB_NAME} in src/lib/curl/lib/CMakeLists.txt
  set(CURL_LIBRARIES "libcurl")
else()
  find_package(CURL REQUIRED)
endif()
cmake_print_variables(CMAKE_CURRENT_LIST_FILE CURL_LIBRARIES)

find_package(OpenSSL REQUIRED)

add_subdirectory(lib/platform)
add_subdirectory(lib/ontrac)
add_subdirectory(lib/spil_os)
add_subdirectory(lib/spil_file)

option(SPIL_CRYPT_USE_TLS "include TLS support in spil_crypt" OFF) # TLS is not handled by spil_crypt
add_definitions(-DUSE_SYS_REALTIME_WO_TIME_SERVER)  # Do not use time server for trusted time
add_subdirectory(lib/spil_crypt)
set_property(TARGET ${EMOOTA3}spil_crypt PROPERTY POSITION_INDEPENDENT_CODE TRUE)

set(${PROJECT_NAME}_SOURCES
        src/abst.c
        src/abst_crypt.c
        src/abst_image.c
        src/https-post_cacertinmem.c
        )

add_library(${PROJECT_NAME} STATIC ${${PROJECT_NAME}_SOURCES})

#Behavior we want is that whenever the "include files change", we
#re-copy them to the build-tree destination.
target_include_directories(${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}> )
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SRC_DIR}/src)

target_link_libraries(${PROJECT_NAME} PRIVATE ${CURL_LIBRARIES} ${EMOOTA3}utility ${EMOOTA3}platform ${EMOOTA3}spil_file ${EMOOTA3}spil_os ${EMOOTA3}spil_crypt)
if (DEFINED INSTALL_BASE)
    target_compile_definitions(${PROJECT_NAME} PUBLIC INSTALL_BASE="${INSTALL_BASE}")
endif ()
if (${CMAKE_BUILD_TYPE} MATCHES Debug)
    target_compile_definitions(${PROJECT_NAME} PUBLIC DEBUG_BUILD)
endif ()

if(${TESTRUNNER})
    target_link_libraries(${PROJECT_NAME} PRIVATE  cmocka)
    target_compile_definitions(${PROJECT_NAME} PUBLIC TESTRUNNER)
endif()

