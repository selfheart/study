# TMC CONFIDENTIAL
# $TUSLibId$
# Copyright (C) 2022 TOYOTA MOTOR CORPORATION
# All Rights Reserved.

# helper to build LUa extentions

function (add_lua_extension ARG)
  add_library(${ARG} SHARED ${ARGN})
  # create "NAME.so", not "libNAME.so"
  set_target_properties(${ARG} PROPERTIES PREFIX "")
  target_include_directories(${ARG} PRIVATE
    ${tus_lua_SOURCE_DIR}
    )
endFunction()
