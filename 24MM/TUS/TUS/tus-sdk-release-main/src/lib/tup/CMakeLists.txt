# TMC CONFIDENTIAL
# $TUSLibId$
# Copyright (C) 2022 TOYOTA MOTOR CORPORATION
# All Rights Reserved.

cmake_minimum_required(VERSION 3.13)

project(tup_parser)

if ("${TUS_LIBCURL}" STREQUAL "module")
  # 'libcurl' is ${LIB_NAME} in src/lib/curl/lib/CMakeLists.txt
  set(CURL_LIBRARIES "libcurl")
else()
  find_package(CURL REQUIRED)
endif()
cmake_print_variables(CMAKE_CURRENT_LIST_FILE CURL_LIBRARIES)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_subdirectory(error_code)
add_subdirectory(lua_wrapper)

add_subdirectory(src/compress)
add_subdirectory(src/crypt)

add_library(${PROJECT_NAME} STATIC
  src/tup_parser.c
  )

set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE True)

target_include_directories(${PROJECT_NAME} PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  )

target_include_directories(${PROJECT_NAME} PRIVATE
  ${CURL_INCLUDE_DIR}
  )

target_link_libraries(${PROJECT_NAME} PRIVATE
  Threads::Threads
  tus_includes
  ${CURL_LIBRARIES}
  tup_compress
  tup_crypt
  tus_logger
  )

add_custom_command(
  TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/pubkey.pem $<TARGET_FILE_DIR:runner>
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/pubkey.pem $<TARGET_FILE_DIR:dc>
  )


install(FILES ./pubkey.pem DESTINATION ${INSTALL_PREFIX_UO} COMPONENT ${COMPONENT_UO})
install(FILES ./pubkey.pem DESTINATION ${INSTALL_PREFIX_DC} COMPONENT ${COMPONENT_DC})
