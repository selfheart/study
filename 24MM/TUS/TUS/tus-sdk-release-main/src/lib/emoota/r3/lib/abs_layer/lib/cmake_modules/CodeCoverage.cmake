# Copyright (c) 2012 - 2015, Lars Bilke
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors
#    may be used to endorse or promote products derived from this software without
#    specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#
#
# 2012-01-31, Lars Bilke
# - Enable Code Coverage
#
# 2013-09-17, Joakim Söderberg
# - Added support for Clang.
# - Some additional usage instructions.
#
# USAGE:

# 0. (Mac only) If you use Xcode 5.1 make sure to patch geninfo as described here:
#      http://stackoverflow.com/a/22404544/80480
#
# 1. Copy this file into your cmake modules path.
#
# 2. Add the following line to your CMakeLists.txt:
#      INCLUDE(CodeCoverage)
#
# 3. Set compiler flags to turn off optimization and enable coverage:
#    SET(CMAKE_CXX_FLAGS "-g -O0 -fprofile-arcs -ftest-coverage")
#        SET(CMAKE_C_FLAGS "-g -O0 -fprofile-arcs -ftest-coverage")
#
# 3. Use the function SETUP_TARGET_FOR_COVERAGE to create a custom make target
#    which runs your test executable and produces a lcov code coverage report:
#    Example:
#        SETUP_TARGET_FOR_COVERAGE(
#                               my_coverage_target  # Name for custom target.
#                               test_driver         # Name of the test driver executable that runs the tests.
#                                                                       # NOTE! This should always have a ZERO as exit code
#                                                                       # otherwise the coverage generation will not complete.
#                               coverage            # Name of output directory.
#                               )
#
#    If you need to exclude additional directories from the report, specify them
#    using the LCOV_REMOVE_EXTRA variable before calling SETUP_TARGET_FOR_COVERAGE.
#    For example:
#
#    set(LCOV_REMOVE_EXTRA "'thirdparty/*'")
#
# 4. Build a Debug build:
#        cmake -DCMAKE_BUILD_TYPE=Debug ..
#        make
#        make my_coverage_target
#
#

# Check prereqs
FIND_PROGRAM( GCOV_PATH gcov )
FIND_PROGRAM( LCOV_PATH lcov )
FIND_PROGRAM( GENHTML_PATH genhtml )
FIND_PROGRAM( GCOVR_PATH gcovr PATHS ${CMAKE_SOURCE_DIR}/tests)

IF(NOT GCOV_PATH)
        MESSAGE(FATAL_ERROR "gcov not found! Aborting...")
ENDIF() # NOT GCOV_PATH

IF("${CMAKE_CXX_COMPILER_ID}" MATCHES "(Apple)?[Cc]lang")
        IF("${CMAKE_CXX_COMPILER_VERSION}" VERSION_LESS 3)
                MESSAGE(FATAL_ERROR "Clang version must be 3.0.0 or greater! Aborting...")
        ENDIF()
ELSEIF(NOT CMAKE_COMPILER_IS_GNUCXX)
        MESSAGE(FATAL_ERROR "Compiler is not GNU gcc! Aborting...")
ENDIF() # CHECK VALID COMPILER

SET(CMAKE_CXX_FLAGS_COVERAGE
    "-g -O0 --coverage -fprofile-arcs -ftest-coverage"
    CACHE STRING "Flags used by the C++ compiler during coverage builds."
    FORCE )
SET(CMAKE_C_FLAGS_COVERAGE
    "-g -O0 --coverage -fprofile-arcs -ftest-coverage"
    CACHE STRING "Flags used by the C compiler during coverage builds."
    FORCE )
SET(CMAKE_EXE_LINKER_FLAGS_COVERAGE
    ""
    CACHE STRING "Flags used for linking binaries during coverage builds."
    FORCE )
SET(CMAKE_SHARED_LINKER_FLAGS_COVERAGE
    ""
    CACHE STRING "Flags used by the shared libraries linker during coverage builds."
    FORCE )
MARK_AS_ADVANCED(
    CMAKE_CXX_FLAGS_COVERAGE
    CMAKE_C_FLAGS_COVERAGE
    CMAKE_EXE_LINKER_FLAGS_COVERAGE
    CMAKE_SHARED_LINKER_FLAGS_COVERAGE )

IF ( NOT (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "Coverage"))
  MESSAGE( WARNING "Code coverage results with an optimized (non-Debug) build may be misleading" )
ENDIF() # NOT CMAKE_BUILD_TYPE STREQUAL "Debug"


DEFINE_PROPERTY(GLOBAL PROPERTY COVERAGE_TARGETS
  BRIEF_DOCS "list of targets created by SETUP_TARGET_FOR_COVERAGE."
  FULL_DOCS "Global comma seperated list of targets created by SETUP_TARGET_FOR_COVERAGE.")
# Initialize at most once.
SET(_COVERAGE_TARGETS_ADDED "" CACHED INTERNAL)
IF (NOT _COVERAGE_TARGETS_ADDED)
  SET_PROPERTY(GLOBAL PROPERTY COVERAGE_TARGETS "")
  SET(_COVERAGE_TARGETS_ADDED 1 CACHED INTERNAL FORCE)
ENDIF()

# Param _targetname     The name of new the custom make target
# Param _testrunner     The name of the target which runs the tests.
#                                               MUST return ZERO always, even on errors.
#                                               If not, no coverage report will be created!
# Param _outputname     lcov output is generated as _outputname.info
#                       HTML report is generated in _outputname/index.html
# Optional fourth parameter is passed as arguments to _testrunner
#   Pass them in list form, e.g.: "-j;2" for -j 2
FUNCTION(SETUP_TARGET_FOR_COVERAGE _targetname _testrunner _outputname)

        IF(NOT LCOV_PATH)
                MESSAGE(FATAL_ERROR "lcov not found! Aborting...")
        ENDIF() # NOT LCOV_PATH

        IF(NOT GENHTML_PATH)
                MESSAGE(FATAL_ERROR "genhtml not found! Aborting...")
        ENDIF() # NOT GENHTML_PATH

        GET_FILENAME_COMPONENT(full_output_path ${_outputname} REALPATH BASE_DIR ${CMAKE_BINARY_DIR})
        GET_FILENAME_COMPONENT(output_file_name ${full_output_path} NAME )

	    SET(coverage_info "${full_output_path}/${output_file_name}.info")
	    SET(coverage_baseline "${coverage_info}.baseline")
	    SET(coverage_raw "${coverage_info}.raw")
	    SET(coverage_cleaned "${coverage_info}.cleaned")
	    SEPARATE_ARGUMENTS(test_command UNIX_COMMAND "${_testrunner}")

        #When target is compiled with profiling flags, the object code
        #icludes extra links to profiling code.
        #
        #This extra code, at runtime, drops runtime-profiling data files
        #to disk...where those files are droped to disk is controlled by
        #two ENV variables:
        #
        # * GCOV_PREFIX contains the prefix to add to the absolute paths in the
        # object file.  Prefix can be absolute, or relative.  The default is
        # no prefix.
        #
        # * GCOV_PREFIX_STRIP indicates the how many initial directory names to
        # strip off the hardwired absolute paths.  Default value is 0.
        #
        # _Note:_ If GCOV_PREFIX_STRIP is set without GCOV_PREFIX is
        # undefined, then a relative path is made out of the hardwired
        # absolute paths.
        #  For example, if the object file '/user/build/foo.o' was built with
        # '-fprofile-arcs', the final executable will try to create the data file
        # '/user/build/foo.gcda' when running on the target system.  This will
        # fail if the corresponding directory does not exist and it is unable to
        # create it.  This can be overcome by, for example, setting the
        # environment as 'GCOV_PREFIX=/target/run' and 'GCOV_PREFIX_STRIP=1'.
        # Such a setting will name the data file '/target/run/build/foo.gcda'.

        IF( NOT DEFINED CMAKE_BINARY_DIR)
          # Not sure how this would ever happen, maybe if project(..) is never invoked.
          # But lets be defensive anyways.
          MESSAGE(FATAL_ERROR "WTF where is CMAKE_BINARY_DIR?")
        ENDIF()
        # Setup target
        ADD_CUSTOM_TARGET(${_targetname}

	        # Initialize for baseline
                ${LCOV_PATH} --directory ${CMAKE_BINARY_DIR} --capture --initial --output-file ${coverage_baseline}

                # Run tests
                COMMAND ${test_command} ${ARGV3}

		# Capturing lcov counters and generating report
		COMMAND ${LCOV_PATH} --directory ${CMAKE_BINARY_DIR} --capture --output-file ${coverage_info}
		COMMAND ${LCOV_PATH} --add-tracefile ${coverage_baseline} --add-tracefile ${coverage_info} --output-file ${coverage_raw}
		COMMAND ${LCOV_PATH} --remove ${coverage_raw} 'tests/*' '/usr/*' ${LCOV_REMOVE_EXTRA} --output-file ${coverage_cleaned}
		COMMAND ${GENHTML_PATH} -o ${_outputname} ${coverage_cleaned} --branch-coverage
		#COMMAND ${CMAKE_COMMAND} -E remove ${coverage_info} ${coverage_cleaned}

                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Resetting code coverage counters to zero, processing code coverage counters & generating report."
        )
        # Append to target to list of "coverage targets".
        SET_PROPERTY(GLOBAL APPEND PROPERTY COVERAGE_TARGETS "${_targetname}")
        # Another way to keep track of "coverage targets".
        SET_PROPERTY(TARGET ${_targetname} APPEND PROPERTY LABELS "coverage")

        ADD_CUSTOM_COMMAND(TARGET ${_targetname} POST_BUILD
          COMMAND ;
          COMMENT "Open ${_outputname}/index.html in your browser to view the coverage report."
        )
      #TODO: Mon Apr 6 10:08:03 PDT 2020 Should this have a "cleanup"
      # target that is dependent on the built-in <clean> target?

ENDFUNCTION()  # SETUP_TARGET_FOR_COVERAGE

# Param _targetname     The name of new the custom make target
# Param _testrunner     The name of the target which runs the tests
# Param _outputname     cobertura output is generated as _outputname.xml
# Optional fourth parameter is passed as arguments to _testrunner
#   Pass them in list form, e.g.: "-j;2" for -j 2
FUNCTION(SETUP_TARGET_FOR_COVERAGE_COBERTURA _targetname _testrunner _outputname)

        IF(NOT PYTHON_EXECUTABLE)
                MESSAGE(FATAL_ERROR "Python not found! Aborting...")
        ENDIF() # NOT PYTHON_EXECUTABLE

        IF(NOT GCOVR_PATH)
                MESSAGE(FATAL_ERROR "gcovr not found! Aborting...")
        ENDIF() # NOT GCOVR_PATH

        ADD_CUSTOM_TARGET(${_targetname}

                # Run tests
                ${_testrunner} ${ARGV3}

                # Running gcovr
                COMMAND ${GCOVR_PATH} -x -r ${CMAKE_SOURCE_DIR} -e '${CMAKE_SOURCE_DIR}/tests/'  -o ${_outputname}.xml
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Running gcovr to produce Cobertura code coverage report."
        )

        # Show info where to find the report
        ADD_CUSTOM_COMMAND(TARGET ${_targetname} POST_BUILD
                COMMAND ;
                COMMENT "Cobertura code coverage report saved in ${_outputname}.xml."
        )

ENDFUNCTION() # SETUP_TARGET_FOR_COVERAGE_COBERTURA
