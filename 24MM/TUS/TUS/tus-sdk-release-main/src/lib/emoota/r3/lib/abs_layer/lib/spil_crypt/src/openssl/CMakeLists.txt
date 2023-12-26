cmake_minimum_required(VERSION 3.13)
project(${EMOOTA3}crypt_openssl)

message(STATUS "Build SecurityManager with pure OpenSSL")

set(OPENSSL_CMN_IMP_DIR ${PROJECT_SOURCE_DIR})

if(SPIL_CRYPT_USE_TLS)
set(TLS_SRC ${OPENSSL_CMN_IMP_DIR}/crypt_tls.c)
endif()

set(SOURCE_FILES
        ${OPENSSL_CMN_IMP_DIR}/crypt_cipher.c
        ${OPENSSL_CMN_IMP_DIR}/crypt_rng.c
        ${OPENSSL_CMN_IMP_DIR}/crypt_plat.c
        ${OPENSSL_CMN_IMP_DIR}/crypt_hash.c
        ${OPENSSL_CMN_IMP_DIR}/trustchain.c
	${OPENSSL_CMN_IMP_DIR}/master_key.c
        ${TLS_SRC}
        )

add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} ${EMOOTA3}spil_crypt)

set(CRYPT_PLAT_LIBS ssl crypto ${EMOOTA3}spil_file ${PROJECT_NAME} PARENT_SCOPE)

if(COMMAND define_filename_macro_for_sources)
    define_filename_macro_for_sources(${PROJECT_NAME})
endif()
