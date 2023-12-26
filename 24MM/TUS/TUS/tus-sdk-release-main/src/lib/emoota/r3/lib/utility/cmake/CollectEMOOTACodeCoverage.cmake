# -*- mode: cmake; -*-
#[=======================================================================[.rst:
AIRBIQUITY INC. PROPRIETARY AND CONFIDENTIAL INFORMATION

The software and information contained herein is the proprietary and
confidential property of Airbiquity Inc. and shall not be disclosed
in whole or in part. Possession, use, reproduction or transfer of
this software and information is prohibited without the express written
permission of Airbiquity Inc.

Copyright © 2021 Airbiquity Inc.  All rights reserved.


Intended to be run from the commandline via

.. code-block:: shell

$> /usr/bin/cmake -DEMOOTA_COVERAGE_ARCHIVE:FILEPATH="./src/tarball.bzip"      \
                  -DEMOOTA_COVERAGE_SOURCE_DIR:PATH="./path/to/source/"    \
                  -DEMOOTA_COVERAGE_BUILD_DIR:PATH="../configured/build"   \
                  -P CollectEMOOTACodeCoverage.cmake

#]=======================================================================]
#-DEMOOTA_TEST_INFO:FILEPATH="./test_desc.txt"       \
set(PREFIX "EMOOTACodeCoverage")

macro(notify)
  message(STATUS "${PREFIX}: ${ARGV}")
endmacro()

if (NOT COMMAND dump_all_cmake_variables)
  function(dump_all_cmake_variables)
    get_cmake_property(_variableNames VARIABLES)
    foreach (_variableName ${_variableNames})
      notify("${_variableName}=${${_variableName}}")
    endforeach()
  endfunction()
endif()

option(DEBUG_EMOOTA_COVERAGE "Adavanced; Debug the execution of code coverage collection." FALSE)
mark_as_advanced(DEBUG_EMOOTA_COVERAGE)
set(EMOOTA_COVERAGE_ARCHIVE "coverage.tgz"
    CACHE FILEPATH
    "filename of archive to create when code coverage is collected and archived.")

set(EMOOTA_COVERAGE_BUILD_DIR "./"
    CACHE PATH
    "Directory containing build artifacts to search for coverage data within.")

set(EMOOTA_COVERAGE_SOURCE_DIR ""
    CACHE PATH
    "Directory containing source files that coverage data maps. THIS MUST BE SET!")

if (EMOOTA_COVERAGE_SOURCE_DIR STREQUAL "")
  if (DEBUG_EMOOTA_COVERAGE)
    dump_all_cmake_variables()
  endif()
  message(FATAL_ERROR "EMOOTA_COVERAGE_SOURCE_DIR Must be set.")
endif()

if (EXISTS ${EMOOTA_COVERAGE_ARCHIVE})
  notify("TODO:Removing previous coverage archive")
  #TODO: Actual removal.
endif()

# Can do any checks to assert in <script_mode>?
# Probably want to assert not cross-compiling either.
include(${CMAKE_CURRENT_LIST_DIR}/CTestCoverageCollectLCOV.cmake)

#Since this is invoked in script mode, we don't have access to the
#Project() config to know when we are cross-compiling or not.  This
#mechanism does not work for builds configured to target non-native
#targets.
#message(AUTHOR_WARNING "TODO: Collecting coverage from a cross-target'd build is not supported.")

message(STATUS "Collecting code coverage data")
# ``TARBALL <tarfile>``
#   Specify the location of the ``.tar`` file to be created.

# ``SOURCE <source_dir>``
#   Specify the top-level source directory for the build.
#   Default is the value of :variable:`CTEST_SOURCE_DIRECTORY`.

# ``BUILD <build_dir>``
#   Specify the top-level build directory for the build.
#   Default is the value of :variable:`CTEST_BINARY_DIRECTORY`.

# ``DELETE``
#   Delete coverage files after they've been packaged into the .tar.


if (NOT DEFINED EMOOTA_TEST_INFO)

  #Look to try and obtain test info from environment .
  if ((DEFINED ENV{EMOOTA_TEST_NAME}) AND
      (DEFINED ENV{EMOOTA_TEST_DESCRIPTION}))

      ctest_coverage_collect_lcov(TARBALL ${EMOOTA_COVERAGE_ARCHIVE}
        SOURCE ${EMOOTA_COVERAGE_SOURCE_DIR}
        BUILD ${EMOOTA_COVERAGE_BUILD_DIR}
        TEST_INFO
        NAME $ENV{EMOOTA_TEST_NAME}
        DESCRIPTION $ENV{EMOOTA_TEST_DESCRIPTION} )

    else()

      #call without test info
      ctest_coverage_collect_lcov(
        TARBALL ${EMOOTA_COVERAGE_ARCHIVE}
        SOURCE ${EMOOTA_COVERAGE_SOURCE_DIR}
        BUILD ${EMOOTA_COVERAGE_BUILD_DIR} )

  endif()


else()

  #call with test info
  ctest_coverage_collect_lcov(
    TARBALL ${EMOOTA_COVERAGE_ARCHIVE}
    SOURCE ${EMOOTA_COVERAGE_SOURCE_DIR}
    BUILD ${EMOOTA_COVERAGE_BUILD_DIR}
    TEST_INFO FILE ${EMOOTA_TEST_INFO} )

endif()
