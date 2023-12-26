# TMC CONFIDENTIAL
# $TUSLibId$
# Copyright (C) 2022 TOYOTA MOTOR CORPORATION
# All Rights Reserved.

# "VERSION_GREATER_EQUAL" requires 3.7
cmake_minimum_required(VERSION 3.13)
project(${EMOOTA3}spil_crypt)

option(SPIL_CRYPT_USE_TLS "include TLS support in spil_crypt" ON)
option(ALLOW_KEY_PROVISION "Allow master key to be provisioned" OFF)

if (LOAD_ROOT_CA_FROM_FS)
# invoke cmake with "-DLOAD_ROOT_CA_FROM_FS=TRUE" to load root CA from
#     <otmatic config dir>/rootca.pem
# instead of hard coded root CA
add_definitions(-DCRYPT_LOAD_ROOTCA_FROM_FILE)
endif()

if (NO_RESERVED_THREAD)
  # invoke cmake with "-DNO_PLATFORM=TRUE" to specify no reserved thread
  add_definitions(-DIMPL_RESERVED_THREAD_NUM=0)
endif ()

#add_definition(-DCRYPT_USE_CUSTOMIZED_MUTEX) # use mutex specified by caller
#add_definition(-DTRUST_CHAIN_VERIFY_CERT) # use trust chain to verify cert explicitly, other than TLS
#add_definition(-DEXTRACT_CERT) # include interface to extract public key from cert


if(ALLOW_KEY_PROVISION)
  message(STATUS "Allow master key to be provisioned")
  add_definitions(-DALLOW_KEY_PROVISION)
else()
  message(STATUS "Do NOT allow master key to be provisioned")
endif()

# TODO Currently server does not support ECC when encryption is turned ON
if(FALSE) #EXISTS "$ENV{SDKTARGETSYSROOT}/optee_client")
  set(CRYPT_PLAT "tee-st")
  option(CRYPT_ST_USE_OPENSSL "Use OpenSSL on ST" ON)
  if(CRYPT_ST_USE_OPENSSL)
    set(USR_MBEDTLS FALSE)
  else()
    set(USR_MBEDTLS TRUE)
  endif()

else()
  if (TRUE) # MbedTLS support has been deopped
    FIND_PACKAGE(OpenSSL)
    if(NOT OPENSSL_FOUND)
      message(FATAL_ERROR "Please run: sudo apt-get install libssl-dev")
    else()

      if(${OPENSSL_VERSION} VERSION_GREATER_EQUAL "1.0.2")
	set(CRYPT_PLAT "openssl")
      else()
	message(FATAL_ERROR "OPENSSL version <1.0.2 not supported")
      endif()
    endif()
  endif()
endif()

if(BUILD_WITH_MBEDTLS AND NOT BUILD_WITH_OPENSSL) #default.
  set(CRYPT_PLAT "mbedtls")
  set(USR_MBEDTLS TRUE)
endif()

option(USE_OLD_SDP_TIMESERVER "Use traditional SDP time server" OFF)
if(USE_OLD_SDP_TIMESERVER)
  set(TIMESERVER_SRC crypt_otamatic_sdp_time.c)
endif()

if(USR_MBEDTLS)
  ##
  # Use MBEDTLS crypto library instead of OPENSSL:
  # 1. MBEDTLS is smaller and lighter weight
  # 2. Not all platform provides OPENSSL anyway
  # 3. MBEDTLS backed by ARM
  # 4. Our TEE Trusted Application is using MBEDTLS already, for the same reasons above.
  set(MBEDTLS_SUBDIR "mbedtls")
  get_filename_component(MBEDTLS_DIR "${PROJECT_SOURCE_DIR}/lib/${MBEDTLS_SUBDIR}" REALPATH)
  message(STATUS "linking: ${MBEDTLS_DIR}")
  option(ENABLE_TESTING "do not build mbedtls test suite" OFF)
  option(ENABLE_PROGRAMS "do not build mbedtls programs" OFF)
  option(USE_STATIC_MBEDTLS_LIBRARY "statically link mbedtls" ON)
  option(USE_SHARED_MBEDTLS_LIBRARY "statically link mbedtls" OFF)

if (${OTAMATIC_PLAT_OS} MATCHES "freertos")
else()
  # default to POSIX
  add_definitions(-DUSE_POSIX_RNG_AS_ENTROPY)
  set(USE_SEED_FILE ON)
  add_definitions(-DUSE_SEED_FILE)
endif()

  find_package(Perl)
  if(PERL_FOUND)
    # Unset some of the default settings
    execute_process(COMMAND ${PERL_EXECUTABLE} ${MBEDTLS_DIR}/scripts/config.pl -f ${MBEDTLS_DIR}/include/mbedtls/config.h unset MBEDTLS_SSL_CBC_RECORD_SPLITTING)
    execute_process(COMMAND ${PERL_EXECUTABLE} ${MBEDTLS_DIR}/scripts/config.pl -f ${MBEDTLS_DIR}/include/mbedtls/config.h unset MBEDTLS_SSL_PROTO_TLS1)
    execute_process(COMMAND ${PERL_EXECUTABLE} ${MBEDTLS_DIR}/scripts/config.pl -f ${MBEDTLS_DIR}/include/mbedtls/config.h unset MBEDTLS_SSL_PROTO_TLS1_1)
    execute_process(COMMAND ${PERL_EXECUTABLE} ${MBEDTLS_DIR}/scripts/config.pl -f ${MBEDTLS_DIR}/include/mbedtls/config.h unset MBEDTLS_SSL_PROTO_SSL3)

    execute_process(COMMAND ${PERL_EXECUTABLE} ${MBEDTLS_DIR}/scripts/config.pl -f ${MBEDTLS_DIR}/include/mbedtls/config.h set MBEDTLS_NO_PLATFORM_ENTROPY)
    execute_process(COMMAND ${PERL_EXECUTABLE} ${MBEDTLS_DIR}/scripts/config.pl -f ${MBEDTLS_DIR}/include/mbedtls/config.h unset MBEDTLS_ENTROPY_NV_SEED)
    execute_process(COMMAND ${PERL_EXECUTABLE} ${MBEDTLS_DIR}/scripts/config.pl -f ${MBEDTLS_DIR}/include/mbedtls/config.h unset MBEDTLS_TIMING_C)
if (NOT USE_SEED_FILE)
    execute_process(COMMAND ${PERL_EXECUTABLE} ${MBEDTLS_DIR}/scripts/config.pl -f ${MBEDTLS_DIR}/include/mbedtls/config.h unset MBEDTLS_FS_IO)
endif()
    execute_process(COMMAND ${PERL_EXECUTABLE} ${MBEDTLS_DIR}/scripts/config.pl -f ${MBEDTLS_DIR}/include/mbedtls/config.h unset MBEDTLS_NET_C)
    execute_process(COMMAND ${PERL_EXECUTABLE} ${MBEDTLS_DIR}/scripts/config.pl -f ${MBEDTLS_DIR}/include/mbedtls/config.h unset MBEDTLS_DEPRECATED_WARNING)
    execute_process(COMMAND ${PERL_EXECUTABLE} ${MBEDTLS_DIR}/scripts/config.pl -f ${MBEDTLS_DIR}/include/mbedtls/config.h unset MBEDTLS_HAVEGE_C)
    execute_process(COMMAND ${PERL_EXECUTABLE} ${MBEDTLS_DIR}/scripts/config.pl -f ${MBEDTLS_DIR}/include/mbedtls/config.h unset MBEDTLS_THREADING_PTHREAD)

    execute_process(COMMAND ${PERL_EXECUTABLE} ${MBEDTLS_DIR}/scripts/config.pl -f ${MBEDTLS_DIR}/include/mbedtls/config.h set MBEDTLS_PLATFORM_TIME_ALT)
  endif()

  add_subdirectory("${MBEDTLS_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/${MBEDTLS_SUBDIR}_lib")
  include_directories("${MBEDTLS_DIR}/include")
endif()

add_subdirectory(src/${CRYPT_PLAT})
set_property(TARGET ${EMOOTA3}crypt_openssl PROPERTY POSITION_INDEPENDENT_CODE TRUE)

set(SOURCE_FILES
  ${CRYPT_LIB_FILES}
  src/crypt_cmn.c
  src/crypt_mutex.c
  ${TIMESERVER_SRC}
  )

add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES})

if (${OTAMATIC_PLAT_OS} MATCHES "freertos")
  target_link_libraries(${PROJECT_NAME}
    ${EMOOTA3}ontrac ${EMOOTA3}platform ${EMOOTA3}spil_os ${EMOOTA3}spil_file freertos ${CRYPT_PLAT_LIBS})
else()
  target_link_libraries(${PROJECT_NAME}
    ${EMOOTA3}ontrac ${EMOOTA3}platform ${EMOOTA3}spil_os ${EMOOTA3}spil_file ${CRYPT_PLAT_LIBS})
endif()

target_include_directories(${PROJECT_NAME} PUBLIC
  "${PROJECT_SOURCE_DIR}/include"
  )

if(COMMAND define_filename_macro_for_sources)
  define_filename_macro_for_sources(${PROJECT_NAME})
endif()

if(BUILD_${PROJECT_NAME}_TESTS)
  if( NOT ${CMAKE_VERSION} VERSION_GREATER "3.7")
    message(SEND_ERROR "CMake Version dependency not met for ${PROJECT_NAME} Tests")
  endif()
  list(APPEND CMAKE_MODULE_PATH "${libraries_DIR}/cmake_modules")
  include(CTest)
  enable_testing()
  add_subdirectory(test)
endif()
