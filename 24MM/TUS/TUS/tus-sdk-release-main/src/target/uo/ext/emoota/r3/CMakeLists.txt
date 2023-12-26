# TMC CONFIDENTIAL
# $TUSLibId$
# Copyright (C) 2022 TOYOTA MOTOR CORPORATION
# All Rights Reserved.

cmake_minimum_required(VERSION 3.13)
project(sdpv3)

add_library(${PROJECT_NAME} SHARED
  ./wrapper.c
  ./cli_lib.c
   t1_layer/activate.c
   t1_layer/download.c
   t1_layer/getcmd.c
   t1_layer/send_event.c
   t1_layer/get_campaign.c
   t1_layer/store_trust_root.c
  )

target_include_directories(${PROJECT_NAME}
  PRIVATE
  ${tus_lua_SOURCE_DIR}
  )

target_link_libraries(${PROJECT_NAME}
  emoota3_utility
  emoota3_abs_layer
  emoota3_uptane
  )

install(TARGETS ${PROJECT_NAME}
  DESTINATION ${INSTALL_PREFIX_UO} COMPONENT ${COMPONENT_UO})
