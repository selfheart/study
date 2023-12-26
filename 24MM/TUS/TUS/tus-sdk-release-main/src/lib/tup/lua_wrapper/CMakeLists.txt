# TMC CONFIDENTIAL
# $TUSLibId$
# Copyright (C) 2022 TOYOTA MOTOR CORPORATION
# All Rights Reserved.

cmake_minimum_required(VERSION 3.13)

project(tup_parser_wrapper)

add_library(${PROJECT_NAME} SHARED
  ${CMAKE_CURRENT_SOURCE_DIR}/tup_parser_wrapper.c
)

set_target_properties(${PROJECT_NAME}
  PROPERTIES
    OUTPUT_NAME "tup_parser"
    PREFIX ""
  )

target_include_directories(${PROJECT_NAME} PRIVATE
  ${tus_lua_SOURCE_DIR}
  )

target_link_libraries(${PROJECT_NAME}
  tup_parser
  )

add_custom_command(
  TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/tup_parser.so $<TARGET_FILE_DIR:runner>
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/tup_parser.so $<TARGET_FILE_DIR:dc>
  )


install(TARGETS ${PROJECT_NAME}
  DESTINATION ${INSTALL_PREFIX_UO} COMPONENT ${COMPONENT_UO})
install(TARGETS ${PROJECT_NAME}
  DESTINATION ${INSTALL_PREFIX_DC} COMPONENT ${COMPONENT_DC})
