# TMC CONFIDENTIAL
# $TUSLibId$
# Copyright (C) 2022 TOYOTA MOTOR CORPORATION
# All Rights Reserved.

cmake_minimum_required(VERSION 3.13)

project(tup_crypt)

if ("${TUS_CRYPT_LIBRARY}" STREQUAL "mbedtls")
  add_subdirectory(mbedtls)
  set(TUP_CRYPT_LIBS "tup_mbedtls")
else()
  add_subdirectory(openssl)
  set(TUP_CRYPT_LIBS "tup_openssl")
endif()

add_library(${PROJECT_NAME} STATIC
  tup_crypt.c
  )

set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE True)

target_include_directories(${PROJECT_NAME} PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  $<TARGET_PROPERTY:tup_parser,INCLUDE_DIRECTORIES>
  )

target_link_libraries(${PROJECT_NAME} PRIVATE
  ${TUP_CRYPT_LIBS}
  tus_logger
  )
