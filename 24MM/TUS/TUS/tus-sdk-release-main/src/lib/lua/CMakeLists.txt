# TMC CONFIDENTIAL
# $TUSLibId$
# Copyright (C) 2022 TOYOTA MOTOR CORPORATION
# All Rights Reserved.

cmake_minimum_required(VERSION 3.13)

project(tus_lua)

# prepend "tus" to avoid conflictiong system's Lua
# append "_static" to avoid confusion when linked by "-llua"
add_library(tus_lua SHARED
  # 'CORE' srcs (see 'CORE_O' in ./Makefile)
  lapi.c
  lcode.c
  lctype.c
  ldebug.c
  ldo.c
  ldump.c
  lfunc.c
  lgc.c
  llex.c
  lmem.c
  lobject.c
  lopcodes.c
  lparser.c
  lstate.c
  lstring.c
  ltable.c
  ltablib.c
  ltm.c
  lundump.c
  lvm.c
  lzio.c
  # 'LIB' srcs, some of them may be removed
  lauxlib.c
  lbaselib.c
  lcorolib.c
  ldblib.c
  linit.c
  liolib.c
  lmathlib.c
  loadlib.c
  loslib.c
  lstrlib.c
  lutf8lib.c
  )

# note: no "LUA_COMPAT_5_*" shall be needed here
# fixme: set LUA_PATH_DEFAULT and LUA_CPATH_DEFAULT (and LUA_DIRSEP), too?
target_compile_definitions(tus_lua PRIVATE LUA_USE_POSIX LUA_USE_DLOPEN)

# make a static version of liblua using the same configuration
add_library(tus_lua_static STATIC
  $<TARGET_PROPERTY:tus_lua,SOURCES>
  )
target_compile_definitions(tus_lua_static PRIVATE
  $<TARGET_PROPERTY:tus_lua,COMPILE_DEFINITIONS>
  )
target_link_libraries(tus_lua_static PUBLIC
  -lm
  ${CMAKE_DL_LIBS}
  )

# name the executable as "lua", not "tus_lua" for convenience
add_executable(lua
  lua.c
  )
target_link_libraries(lua
  tus_lua
  -lm
  ${CMAKE_DL_LIBS}
  )
target_compile_definitions(lua PRIVATE
  $<TARGET_PROPERTY:tus_lua,COMPILE_DEFINITIONS>
  )
