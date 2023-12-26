

get_filename_component(TEST_DRIVER_TEMPLATE  "${CMAKE_CURRENT_LIST_DIR}/test-driver.sh.in" ABSOLUTE)
if (EXISTS ${TEST_DRIVER_TEMPLATE})
  set( _EMOOTA_TEST_DRIVER_TEMPLATE "${TEST_DRIVER_TEMPLATE}" CACHE STRING "Path to test driver template." FORCE)
else()
  message(WARNING "Unable to locate user specified test driver template.")
endif()
unset(TEST_DRIVER_TEMPLATE)

if(EMOOTA_BUILD_TESTS)
  set(EMOOTA_INCLUDE_TESTS ON)
endif()

if (NOT DEFINED EMOOTA_TEST_COVERAGE)
  option(EMOOTA_TEST_COVERAGE "Measure test coverage." OFF)
endif(NOT DEFINED EMOOTA_TEST_COVERAGE)

if (EMOOTA_TEST_COVERAGE)
  #Not specializing based on compiler ID
  set(GCC_COVERAGE_FLAGS "-O0 -g -fprofile-arcs -ftest-coverage"
    CACHE STRING "Flags added to the GCC compiler flags when EMOOTA_TEST_COVERAGE is set.")
  set(CMAKE_C_FLAGS_DEBUG "${GCC_COVERAGE_FLAGS} ${CMAKE_C_FLAGS}")

  # set(CMAKE_LD_FLAGS_COVERAGE "-fprofile-arcs -ftest-coverage"
  #   CACHE STRING "Flags added to the Linker when EMOOTA_TEST_COVERAGE is set.")
  # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_LD_FLAGS_COVERAGE} ${CMAKE_EXE_LINKER_FLAGS}")

  include(CMakePrintHelpers)
  cmake_print_variables(CMAKE_EXE_LINKER_FLAGS)

  set(test_cov_dir "${CMAKE_CURRENT_BINARY_DIR}/")
  if (DEFINED EMOOTA_TEST_COVERAGE_DIR)
      set(test_cov_dir "${EMOOTA_TEST_COVERAGE_DIR}/")    # use cli supplied value
  elseif (DEFINED ENV{EMOOTA_TEST_COVERAGE_DIR} )
    set(test_cov_dir "$ENV{EMOOTA_TEST_COVERAGE_DIR}/")        # find from environment
  else()
    message(STATUS "Falling back to default EMOOTA_TEST_COVERAGE_DIR: ${test_cov_dir}")
  endif()
  set(EMOOTA_TEST_COVERAGE_DIR "${test_cov_dir}"
    CACHE PATH "Output location for builds configured for test coverage. Coverage artifacts to be written in this directory." )

  if (NOT EXISTS ${EMOOTA_TEST_COVERAGE_DIR})
    file(MAKE_DIRECTORY "${EMOOTA_TEST_COVERAGE_DIR}")
  endif()
  unset(test_cov_dir)
endif()
