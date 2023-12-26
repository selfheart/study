# to be passed as 'CMAKE_TOOLCHAIN_FILE', ex.
#   cmake -B build -DCMAKE_TOOLCHAIN_FILE=${TUS}etc/cmake/toolchain/qnx710.cmake ${TUS}

if(DEFINED ENV{QNX_TARGET})
  message(STATUS "using QNX toolchain")
else()
  # QNX_TARGET shall have been set by qnxsdp-dev.sh (like $HOME/qnx710/target/qnx7)
  message(FATAL_ERROR "shall source QNX SDP env (qnxsdp-dev.sh)")
endif()

set(CMAKE_SYSTEM_NAME "QNX")
set(QNX_ARCH "aarch64le") ## may choose "armle-v7"
set(CMAKE_COMPILER_TARGET "gcc_nto${QNX_ARCH}")

# may use
#  set(CMAKE_C_COMPILER qcc)
#  set(CMAKE_CXX_COMPILER q++)
# for other QNX versions
set(CMAKE_C_COMPILER aarch64-unknown-nto-qnx7.1.0-gcc)
set(CMAKE_CXX_COMPILER aarch64-unknown-nto-qnx7.1.0-g++)

# see qnx710/target/qnx7/usr/include/sys/platform.h
add_compile_definitions(_QNX_SOURCE=1)
add_compile_definitions(_XOPEN_SOURCE=700)

set(CMAKE_SYSROOT "$ENV{QNX_TARGET}")
set(CMAKE_SYSROOT_LINK "${CMAKE_SYSROOT}/${QNX_ARCH}")
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# build LWS from ${TUS}/src/lib/libwebsockets
set(TUS_LIBWEBSOCKETS "module" CACHE STRING "Provider of libwebsockets")

# build libgrpc from this repository, ignoreing system's
set(TUS_LIBGRPC "module" CACHE STRING "Provider of gRPC library")
