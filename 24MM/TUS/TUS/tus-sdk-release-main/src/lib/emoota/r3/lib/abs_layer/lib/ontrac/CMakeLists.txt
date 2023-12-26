# TMC CONFIDENTIAL
# $TUSLibId$
# Copyright (C) 2022 TOYOTA MOTOR CORPORATION
# All Rights Reserved.

cmake_minimum_required(VERSION 3.13)
project(${EMOOTA3}ontrac)
message(STATUS "project: ${PROJECT_NAME}")

get_filename_component(CWD ${PROJECT_SOURCE_DIR} REALPATH)

set(SOURCE_FILES
		${CWD}/src/ontrac/abq_class.c
		${CWD}/src/ontrac/abq_props.c
		${CWD}/src/ontrac/abq_str.c
		${CWD}/src/ontrac/sorted_set.c
		${CWD}/src/ontrac/mem_usage.c
		${CWD}/src/ontrac/namevars.c
		${CWD}/src/ontrac/primitives.c
		${CWD}/src/ontrac/status_codes.c
		${CWD}/src/ontrac/traverser.c
		${CWD}/src/ontrac/var_list.c
		${CWD}/src/ontrac/var_stack.c
		${CWD}/src/rest/item_reader.c
		${CWD}/src/rest/item_tap.c
		${CWD}/src/rest/json_reader.c
		${CWD}/src/rest/item_writer.c
		${CWD}/src/rest/json_writer.c
		${CWD}/src/stream/datasink.c
		${CWD}/src/stream/datatap.c
		${CWD}/src/stream/limited_tap.c
		${CWD}/src/stream/mem_sink.c
		${CWD}/src/stream/mem_tap.c
		${CWD}/src/stream/mime_types.c
		${CWD}/src/text/ascii.c
		${CWD}/src/text/b64_enc.c
		${CWD}/src/text/codec.c
		${CWD}/src/text/hex_enc.c
		${CWD}/src/text/time_enc.c
		${CWD}/src/text/json_enc.c
		${CWD}/src/text/url_enc.c
		${CWD}/src/text/utf8_enc.c
		${CWD}/src/text/utilities.c
		${CWD}/src/util/byte_buffer.c
		${CWD}/src/util/itsy_bits.c
		${CWD}/src/util/misc_items.c
		${CWD}/src/util/thread_utils.c
		${CWD}/src/util/url_support.c
	)

add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES})
set_property(TARGET ${EMOOTA3}ontrac PROPERTY POSITION_INDEPENDENT_CODE TRUE)

target_link_libraries(${PROJECT_NAME} PUBLIC ${EMOOTA3}platform)
target_include_directories(${PROJECT_NAME}
    PUBLIC ${PROJECT_SOURCE_DIR}/include
    PRIVATE ${TEST}/include)

include(AirbiquityUtils)
define_filename_macro_for_sources(${PROJECT_NAME})

if(TARGET application)
    add_global_compile_definition("HAVE_APPLICATION")
    target_link_libraries(${PROJECT_NAME} PUBLIC application)
endif()

if(TARGET commons)
    add_global_compile_definition("HAVE_SLOG_H")
    target_link_libraries(${PROJECT_NAME} PUBLIC commons)
endif()

if(NOT DEFINED SINGLE_THREADED)
    set(SINGLE_THREADED false)
endif()

# `USE_PTHREAD` used for building hmi_capi lib only
if(NOT DEFINED USE_PTHREAD)
    set(USE_PTHREAD false)
endif()

if(NOT DEFINED DISABLE_DEFAULT_MEM_USAGE)
    add_definitions(-DENABLE_DEFAULT_MEMORY_USAGE)
endif()
