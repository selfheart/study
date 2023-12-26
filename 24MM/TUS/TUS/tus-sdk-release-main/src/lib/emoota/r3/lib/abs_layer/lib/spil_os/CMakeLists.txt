# TMC CONFIDENTIAL
# $TUSLibId$
# Copyright (C) 2022 TOYOTA MOTOR CORPORATION
# All Rights Reserved.

cmake_minimum_required(VERSION 3.13)
project(${EMOOTA3}spil_os)
message(STATUS "project: ${PROJECT_NAME}")

if(BUILD_TESTS AND WITH_COVERAGE)
  set(CMAKE_C_FLAGS "-g -O0 -fprofile-arcs -ftest-coverage ${CMAKE_C_FLAGS}")
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if (${OTAMATIC_PLAT_OS} MATCHES "freertos")
    set(PLAT_SRC
        ${PROJECT_SOURCE_DIR}/src/${OTAMATIC_PLAT_OS}/aqSpilImpl.c
        ${PROJECT_SOURCE_DIR}/src/${OTAMATIC_PLAT_OS}/aqTimeImpl.c
    )
    set(PLAT_INC
        ${PROJECT_SOURCE_DIR}/src/${OTAMATIC_PLAT_OS}/
    )
    set(PLAT_LIBS
        freertos
    )
    add_compile_definitions(IMPL_RESERVED_THREAD_NUM=0U)
else()
    # default as POSIX Linux
    set(OTAMATIC_PLAT_OS posix)
    set(PLAT_SRC
        ${PROJECT_SOURCE_DIR}/src/${OTAMATIC_PLAT_OS}/aqSpilImpl.c
        ${PROJECT_SOURCE_DIR}/src/${OTAMATIC_PLAT_OS}/aqTimeImpl.c
    )
    set(PLAT_INC
        ${PROJECT_SOURCE_DIR}/src/${OTAMATIC_PLAT_OS}/
    )
    set(PLAT_LIBS
        Threads::Threads
    )
    add_compile_definitions(IMPL_RESERVED_THREAD_NUM=2U)
endif()

set(SOURCE_FILES
        ${PLAT_SRC}
        ${PROJECT_SOURCE_DIR}/src/aqSpilWorkerThread.c
)

if(NOT WIN32)
    set(SOURCE_FILES ${SOURCE_FILES}
        ${PROJECT_SOURCE_DIR}/src/aqSpilRauc.c
    )
endif()

if(BUILD_TESTS)
    get_filename_component(TEST ${PROJECT_SOURCE_DIR}/test REALPATH)
    set(SOURCE_FILES ${SOURCE_FILES}
        ${TEST}/src/spil_thread_test.c
        ${TEST}/src/spil_time_test.c
    )
endif()

add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES})
set_property(TARGET ${EMOOTA3}spil_os PROPERTY POSITION_INDEPENDENT_CODE TRUE)

target_include_directories(${PROJECT_NAME}
        PUBLIC ${PROJECT_SOURCE_DIR}/include
        PUBLIC ${freertos_SOURCE_DIR}/abq_src
        PUBLIC ${PLAT_INC}
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC ${EMOOTA3}ontrac ${EMOOTA3}spil_file
    PRIVATE ${PLAT_LIBS}
)

if(BUILD_TESTS)
  target_include_directories(${PROJECT_NAME}
    PRIVATE ${TEST}/include )
  include(CTest)
  enable_testing()
  add_subdirectory(test)
  Message(STATUS " ${PROJECT_NAME}: test subdirectory has been added.")
  # AirbiquityUtils may have been populated via "add_subdirectory(test)"
  include(AirbiquityUtils)
  # Fetch aqmock ONLY IF spil_os is top level of the current CMake source tree.
  if(${CMAKE_SOURCE_DIR} STREQUAL ${PROJECT_SOURCE_DIR})
      if(NOT TARGET aqmock)
        import_sdm_c_module("aqmock" "integration")
      endif ()
  endif ()
else ()
    Message(STATUS " ${PROJECT_NAME}: test subdirectory is NOT added.")
    include(AirbiquityUtils)
endif ()

# function is defined in AirbiquityUtils.cmake
define_filename_macro_for_sources(${PROJECT_NAME})

# directory not available for customer deliveries
if (EXISTS doc)
    add_subdirectory(doc)
endif ()
