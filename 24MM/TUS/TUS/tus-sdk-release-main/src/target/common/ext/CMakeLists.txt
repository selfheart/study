# TMC CONFIDENTIAL
# $TUSLibId$
# Copyright (C) 2022 TOYOTA MOTOR CORPORATION
# All Rights Reserved.

cmake_minimum_required(VERSION 3.13)

# to "include(LuaWrapper)"
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")
cmake_print_variables(CMAKE_MODULE_PATH)

add_subdirectory(delta_update)
add_subdirectory(grpc)
add_subdirectory(protobuf)
