# TMC CONFIDENTIAL
# $TUSLibId$
# Copyright (C) 2022 TOYOTA MOTOR CORPORATION
# All Rights Reserved.

cmake_minimum_required(VERSION 3.13)

if (TUS_UO_PROVIDER_SDPV3)
  add_subdirectory(emoota/r3)
endif()

cmake_print_variables(CMAKE_CURRENT_LIST_FILE TUS_CRYPT_LIBRARY)
if ("${TUS_CRYPT_LIBRARY}" STREQUAL "openssl")
  # default crypt library
elseif ("${TUS_CRYPT_LIBRARY}" STREQUAL "mbedtls")
  cmake_print_variables(CMAKE_CURRENT_LIST_FILE TUS_LIBMBEDTLS)
  if ("${TUS_LIBMBEDTLS}" STREQUAL "module")
    option(ENABLE_PROGRAMS "Build mbed TLS programs." OFF)
    option(ENABLE_TESTING "Build mbed TLS tests." OFF)
    option(USE_STATIC_MBEDTLS_LIBRARY "Build mbed TLS static library." ON)
    option(USE_SHARED_MBEDTLS_LIBRARY "Build mbed TLS shared library." OFF)
    add_subdirectory(mbedtls)
    set_property(TARGET mbedtls PROPERTY POSITION_INDEPENDENT_CODE TRUE)
    set_property(TARGET mbedcrypto PROPERTY POSITION_INDEPENDENT_CODE TRUE)
    set_property(TARGET mbedx509 PROPERTY POSITION_INDEPENDENT_CODE TRUE)
  endif()
else()
  cmake_print_variables(TUS_CRYPT_LIBRARY)
  message(FATAL_ERROR "invalid TUS_CRYPT_LIBRARY")
endif()

cmake_print_variables(CMAKE_CURRENT_LIST_FILE TUS_LIBCURL)
if ("${TUS_LIBCURL}" STREQUAL "module")
  # override default cUURL options
  option(BUILD_CURL_EXE "Set to ON to build curl executable." OFF)
  option(CURL_ENABLE_EXPORT_TARGET "to enable cmake export target" OFF)
  option(CURL_DISABLE_ALTSVC "disables alt-svc support" OFF)
  option(CURL_DISABLE_COOKIES "disables cookies support" OFF)
  option(CURL_DISABLE_CRYPTO_AUTH "disables cryptographic authentication" OFF)
  option(CURL_DISABLE_DICT "disables DICT" ON)
  option(CURL_DISABLE_DOH "disables DNS-over-HTTPS" ON)
  option(CURL_DISABLE_FTP "disables FTP" ON)
  option(CURL_DISABLE_GOPHER "disables Gopher" ON)
  option(CURL_DISABLE_HSTS "disables HSTS support" ON)  # HTTP Strict Transport Security
  option(CURL_DISABLE_HTTP "disables HTTP" OFF)
  option(CURL_DISABLE_HTTP_AUTH "disables all HTTP authentication methods" OFF)
  option(CURL_DISABLE_IMAP "disables IMAP" ON)
  option(CURL_DISABLE_LDAP "disables LDAP" ON)
  option(CURL_DISABLE_LDAPS "disables LDAPS" ON)
  option(CURL_DISABLE_MIME "disables MIME support" OFF)
  option(CURL_DISABLE_MQTT "disables MQTT" ON)  # MQTTv3
  option(CURL_DISABLE_NETRC "disables netrc parser" ON)
  option(CURL_DISABLE_NTLM "disables NTLM support" ON)
  option(CURL_DISABLE_PARSEDATE "disables date parsing" OFF)
  option(CURL_DISABLE_POP3 "disables POP3" ON)
  option(CURL_DISABLE_PROGRESS_METER "disables built-in progress meter" ON)
  option(CURL_DISABLE_PROXY "disables proxy support" ON)
  option(CURL_DISABLE_RTSP "disables RTSP" ON)
  option(CURL_DISABLE_SHUFFLE_DNS "disables shuffle DNS feature" OFF)
  option(CURL_DISABLE_SMB "disables SMB" ON)
  option(CURL_DISABLE_SMTP "disables SMTP" ON)
  option(CURL_DISABLE_SOCKETPAIR "disables use of socketpair for curl_multi_poll" OFF)
  option(CURL_DISABLE_TELNET "disables Telnet" ON)
  option(CURL_DISABLE_TFTP "disables TFTP" ON)
  option(CURL_DISABLE_VERBOSE_STRINGS "disables verbose strings" OFF)

  option(CURL_DISABLE_TESTS "disables tests" ON)

  option(ENABLE_IPV6 "Define if you want to enable IPv6 support" ON)
  option(CURL_ENABLE_SSL "Enable SSL support" ON)
  # ToDo: may enable mutiple SSLs
  if ("${TUS_CRYPT_LIBRARY}" STREQUAL "openssl")
    cmake_dependent_option(CURL_USE_OPENSSL "Use OpenSSL code. Experimental" ON CURL_ENABLE_SSL OFF)
  elseif ("${TUS_CRYPT_LIBRARY}" STREQUAL "mbedtls")
    cmake_dependent_option(CURL_USE_MBEDTLS "Enable mbedTLS for SSL/TLS" ON CURL_ENABLE_SSL OFF)
  endif()
  option(CURL_DISABLE_OPENSSL_AUTO_LOAD_CONFIG "Disable automatic loading of OpenSSL configuration" OFF)
  cmake_dependent_option(CURL_USE_BEARSSL "Enable BearSSL for SSL/TLS" OFF CURL_ENABLE_SSL OFF)
  cmake_dependent_option(CURL_USE_NSS "Enable NSS for SSL/TLS" OFF CURL_ENABLE_SSL OFF)
  cmake_dependent_option(CURL_USE_WOLFSSL "enable wolfSSL for SSL/TLS" OFF CURL_ENABLE_SSL OFF)

  option(USE_NGHTTP2 "Use Nghttp2 library" OFF)
  option(USE_NGTCP2 "Use ngtcp2 and nghttp3 libraries for HTTP/3 support" OFF)

  option(CURL_USE_LIBPSL "Use libPSL" OFF)
  option(CURL_USE_LIBSSH2 "Use libSSH2" OFF)
  option(CURL_USE_LIBSSH "Use libSSH" OFF)

  option(CURL_USE_GSSAPI "Use GSSAPI implementation (right now only Heimdal is supported with CMake build)" OFF)

  option(ENABLE_UNIX_SOCKETS "Define if you want Unix domain sockets support" ON)

  # ToDo: may use TUS-custom dir
  set(CURL_CA_BUNDLE "auto" CACHE STRING
    "Path to the CA bundle. Set 'none' to disable or 'auto' for auto-detection. Defaults to 'auto'.")
  set(CURL_CA_FALLBACK OFF CACHE BOOL
    "Set ON to use built-in CA store of TLS backend. Defaults to OFF")
  set(CURL_CA_PATH "auto" CACHE STRING
    "Location of default CA path. Set 'none' to disable or 'auto' for auto-detection. Defaults to 'auto'.")

  option(CURL_DISABLE_RANDOM_FILE "inhibit using /dev/urandom" ON)
  add_subdirectory(curl)
  # 'libcurl' is ${LIB_NAME} in curl/lib/CMakeLists.txt
  set_property(TARGET libcurl PROPERTY POSITION_INDEPENDENT_CODE TRUE)
endif()


cmake_print_variables(CMAKE_CURRENT_LIST_FILE TUS_LIBGRPC)
if (TUS_LIBGRPC STREQUAL "module")
  # build custom gRPC
  option(gRPC_BUILD_CODEGEN "Build codegen" OFF)
  option(gRPC_BUILD_CSHARP_EXT "Build C# extensions" OFF)
  option(gRPC_BUILD_GRPC_NODE_PLUGIN "Build grpc_node_plugin" OFF)
  option(gRPC_BUILD_GRPC_OBJECTIVE_C_PLUGIN "Build grpc_objective_c_plugin" OFF)
  option(gRPC_BUILD_GRPC_PHP_PLUGIN "Build grpc_php_plugin" OFF)
  option(gRPC_BUILD_GRPC_PYTHON_PLUGIN "Build grpc_python_plugin" OFF)
  option(gRPC_BUILD_GRPC_RUBY_PLUGIN "Build grpc_ruby_plugin" OFF)
  set(gRPC_ABSL_PROVIDER "module" CACHE STRING "Provider of absl library")
  set(gRPC_CARES_PROVIDER "module" CACHE STRING "Provider of c-ares library")
  set(gRPC_PROTOBUF_PROVIDER "module" CACHE STRING "Provider of protobuf library")
  set(gRPC_SSL_PROVIDER "package" CACHE STRING "Provider of ssl library")
  set(gRPC_ZLIB_PROVIDER "package" CACHE STRING "Provider of zlib library")

  # for C-ares (as gRPC's third-party lib)
  OPTION (CARES_STATIC     "Build as a static library"                                             ON)
  OPTION (CARES_SHARED     "Build as a shared library"                                             OFF)
  OPTION (CARES_INSTALL    "Create installation targets (chain builders may want to disable this)" OFF)
  OPTION (CARES_STATIC_PIC "Build the static library as PIC (position independent)"                ON)
  OPTION (CARES_BUILD_TOOLS "Build tools"                                                          OFF)

  # make a shared library to workaround "assertion failed: g_state != nullptr"
  set(TUS_GRPC_NEEDS_SHARED "SHARED")
  add_subdirectory(grpc)
elseif (TUS_LIBGRPC STREQUAL "package")
  # use system-provided gRPC header/libs
  # ToDo: validate gRPC version?
else()
  cmake_print_variables(TUS_LIBGRPC)
  message(FATAL_ERROR "invalid TUS_LIBGRPC")
endif()


cmake_print_variables(CMAKE_CURRENT_LIST_FILE TUS_LIBWEBSOCKETS)
if ("${TUS_LIBWEBSOCKETS}" STREQUAL "module")
  # not using SSL for UO-DC-connention for now
  set(LWS_WITH_NETLINK 0)
  option(LWS_WITH_SSL "Include SSL support (defaults to OpenSSL or similar, mbedTLS if LWS_WITH_MBEDTLS is set)" OFF)
  if ("${TUS_CRYPT_LIBRARY}" STREQUAL "mbedtls")
    option(LWS_WITH_MBEDTLS "Use mbedTLS (>=2.0) replacement for OpenSSL. When setting this, you also may need to specify LWS_MBEDTLS_LIBRARIES and LWS_MBEDTLS_INCLUDE_DIRS" OFF)
    if ("${TUS_LIBMBEDTLS}" STREQUAL "module")
      option(LWS_WITH_EXPORT_LWSTARGETS "Export libwebsockets CMake targets.  Disable if they conflict with an outer cmake project." OFF)
      set(LWS_MBEDTLS_INCLUDE_DIRS "")
      set(LWS_MBEDTLS_LIBRARIES "mbedtls;mbedcrypto;mbedx509")
    endif()
  endif()
  option(LWS_WITHOUT_CLIENT "Don't build the client part of the library" ON)
  option(LWS_WITHOUT_TESTAPPS "Don't build the libwebsocket-test-apps" ON)
  option(LWS_WITHOUT_TEST_SERVER "Don't build the test server" ON)
  option(LWS_WITHOUT_TEST_SERVER_EXTPOLL "Don't build the test server version that uses external poll" ON)
  option(LWS_WITHOUT_TEST_PING "Don't build the ping test application" ON)
  option(LWS_WITHOUT_TEST_CLIENT "Don't build the client test application" ON)
  add_subdirectory(libwebsockets)
endif()


cmake_print_variables(CMAKE_CURRENT_LIST_FILE TUS_LIBLZ4)
if ("${TUS_LIBLZ4}" STREQUAL "module")
  add_subdirectory(lz4)
endif()


# following libs are to be always built from repo.
add_subdirectory(delta_update)
add_subdirectory(lua)
add_subdirectory(posix_errno)
add_subdirectory(tiny-json)
add_subdirectory(tup)

set_property(TARGET tiny_json PROPERTY POSITION_INDEPENDENT_CODE TRUE)
