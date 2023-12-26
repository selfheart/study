# TMC CONFIDENTIAL
# $TUSLibId$
# Copyright (C) 2022 TOYOTA MOTOR CORPORATION
# All Rights Reserved.

cmake_minimum_required(VERSION 3.13)

if (ONLY_HOST_TOOLS)
  message(WARNING "SKIPPING target and library generation for only HOST_TOOLS target")
else()
    add_subdirectory(lib)
    add_subdirectory(target)
endif()

# If tools can be run on target, this condition will be moved to src/tools/CMakeLists.txt
if (CMAKE_CROSSCOMPILING)
  message(WARNING "SKIPPING host tools generation for cross-compile")
else()
  add_subdirectory(tools)
endif()
