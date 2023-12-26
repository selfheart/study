cmake_minimum_required(VERSION 3.13)
project(tiny_json_test_suite_test_suite)

set_directory_properties(PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO )

add_executable(tiny_json_test_suite)
set(tiny_json_test_suite_SOURCES tests.c)
target_sources(tiny_json_test_suite PRIVATE ${tiny_json_test_suite_SOURCES})

target_link_libraries(tiny_json_test_suite PRIVATE tiny_json)
