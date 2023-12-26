# -*- mode: cmake; -*-
cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
include(CMakePrintHelpers)
# File: AddEMOOTA.cmake
#[=======================================================================[.rst:

          AIRBIQUITY INC. PROPRIETARY AND CONFIDENTIAL INFORMATION

   The software and information contained herein is the proprietary and
   confidential property of Airbiquity Inc. and shall not be disclosed
   in whole or in part. Possession, use, reproduction or transfer of
   this software and information is prohibited without the express written
   permission of Airbiquity Inc.

   Copyright © 2021 Airbiquity Inc.  All rights reserved.

AddEMOOTA
------

Add project executables, Test Suites, and Tests.

add_emoota_testsuite
_________________

.. code-block:: cmake

add_emoota_testsuite(<suite-name>
  [ALIAS <runner-name>]
  [EXCLUDE_FROM_CHECK_ALL]  )

Adds a new ``<suite-name>`` Test Suite to the project.  Creates a new
logical target with no build artficat output.  By default nothing
depends on the logical target ``<suite-name>``.  Tests associated to
the Test Suite via the `add_emoota_unittest` command will be kept track
of in the :prop_tgt:`TESTS` property of the ``<suite-name>`` Test
Suite target.

The options are:

``ALIAS``
  When provided, an additional logical target, ``<runner-name>``, is
  added to the project.  ``<runner-name>`` will depend on
  ``<suite-name>`` and when generated, will invoke `ctest <CTest(1)>`
  to run all tests labeled ``<suite-name>``.

  Example:

  .. code-block:: cmake

                  add_emoota_testsuite(subsystem ALIAS check-subsys)

  .. code-block:: shell

                  $ # Assume working directory is a configured build tree.
                  $ cmake --build . --target check-subsys
                  $ # is equivalent to
                  $ ctest -L 'subsys'

  The logical target, ``<runner-name>``, is an added as an
  ``ALIAS`` target. ``ALIAS`` targets are not mutable, installable or
  exportable.  A name can be tested for whether it is an ALIAS name by
  reading the :prop_tgt:`ALIASED_TARGET` property.  Refer to
  :manual:`cmake-buildsystem(7)`.

add_emoota_unittest
________________

.. code-block:: cmake

add_emoota_unittest(<suite-name> <test-target-name>
  [DEPENDS <dep1> [dep2...]]
  [ARGUMENTS <arg1> [arg2...]]
  [LINK_LIBRARIES <lib> [<lib2>]]
  [WORKING_DIRECTORY <dir>]
  [LABELS <label1> [<label2>..]] )

Adds a new executable named ``<test-target-name>`` is to the project,
and associates ``<test-target-name>`` to the Test Suite
``<suite-name>``.  The Test Suite ``<suite-name>`` must exist, it will
not be created.  A new dependency will be added so that
``<suite-name>`` depends on ``<target-name>``.  The Test Suites
associated to ``test-name`` can be read from the :prop_tgt:`SUITE`
property on the ``test-name`` target.

The options are:

``DEPENDS``
  Append the ``DEPENDS`` option values to the dependencies of the
  top-level ``<test-target-name>`` target. Providing a list of values
  to this option is semanticlly equivalient to calling
  :command:`add_dependencies foreach item in the list.

``LINK_LIBRARIES``
  Each ``<lib>`` value supplied to the `LINK_LIBRARIES` option will be
  added to the target `<test-target-name>` PRIVATE linkage.  Each
  ``<lib>`` value can be in one of multiple formats described in
  :command:`<target_link_libraries>`.

``WORKING_DIRECTORY``
  Execute the test `<test-target-name>` with the given current
  working directory.  If it is a relative path it will be interpreted
  relative to the build tree directory corresponding to the current
  source directory.

 ``LABELS``
  Reference :prop_test:`LABELS`.

#]=======================================================================]#
# Save project's policies
cmake_policy(PUSH)
cmake_policy(SET CMP0057 NEW) # if IN_LIST

get_property(prop_def GLOBAL PROPERTY EMOOTA_TESTSUITE DEFINED)
if (NOT prop_def) #Guard, in case also defined at proj-level.
  define_property(GLOBAL PROPERTY EMOOTA_TESTSUITE
    BRIEF_DOCS "List of test suites"
    FULL_DOCS "This property specifies all registered test suites in configured
    build tree.")
endif()
unset(prop_def)

get_property(prop_def GLOBAL PROPERTY EMOOTA_TESTSUITE_RUNNERS DEFINED)
if (NOT prop_def) #Guard, in case also defined at proj-level.
  define_property(GLOBAL PROPERTY EMOOTA_TESTSUITE_RUNNERS
    BRIEF_DOCS "List of targets which execute an associated test suite."
    FULL_DOCS  "This property specifies all registered test suite runners in
    configured build tree.  A test suite runner is a build target which will
     invoke/execute the test suite associated to the runner.")
endif()
unset(prop_def)

#This cache settings could be obtained from a call to Find_package(),
#and use the imported targets found, maybe with a user override?
if ( NOT DEFINED emoota_cmocka_target )
  set( emoota_cmocka_target "cmocka::cmocka" CACHE STRING "Name of CMocka target.")
  mark_as_advanced(emoota_cmocka_target)
endif()

if ( NOT DEFINED _EMOOTA_TEST_DRIVER_TEMPLATE )
  set( _EMOOTA_TEST_DRIVER_TEMPLATE "" CACHE STRING "Path to test driver template.")
  mark_as_advanced(_EMOOTA_TEST_DRIVER_TEMPLATE)
endif()

if ( NOT DEFINED _emoota_test_coverage_script)
  set(_emoota_test_coverage_script
    "${CMAKE_CURRENT_LIST_DIR}/CollectEMOOTACodeCoverage.cmake" CACHE PATH
    "Path on disk of cmake script used to collect test coverage.")
  mark_as_advanced(_emoota_test_coverage_script)
endif()


if(NOT DEFINED EMOOTA_SUPPORTED_TEST_LIBS)
  set(EMOOTA_SUPPORTED_TEST_LIBS "${emoota_cmocka_target}" CACHE INTERNAL "Supported Testing Libraries")
endif()

if (NOT COMMAND dump_all_cmake_variables)
  function(dump_all_cmake_variables)
    get_cmake_property(_variableNames VARIABLES)
    foreach (_variableName ${_variableNames})
      message(STATUS "${_variableName}=${${_variableName}}")
    endforeach()
  endfunction()
endif()

#[========================================[.rst:
add_emoota_executable
________________

.. code-block:: cmake

add_emoota_executable(<target-name>
  [DEPENDS <dep1> [dep2...]]
  [LINK_LIBRARIES <lib1> [<lib2>]]
  [PROPERTIES prop1 value1 [prop2 value2 ...]]  )
#]========================================]
function(add_emoota_executable name)
  message(STATUS "add_emoota_executable: ${ARGN}")
  cmake_parse_arguments( PARSE_ARGV 1 ARG
    "EXCLUDE_FROM_ALL"
    ""
    "DEPENDS;LINK_LIBRARIES;PROPERTIES")

  #TODO: Sure would be nice IF I could forward through any
  #SOURCE_FILES that might have been provided..
  if (ARG_EXCLUDE_FROM_ALL)
    add_executable(${name} EXCLUDE_FROM_ALL)
  else()
    add_executable(${name})
  endif()

  if (ARG_DEPENDS)
    add_dependencies(${name} ${ARG_DEPENDS})
  endif()
  if (ARG_LINK_LIBRARIES)
    target_link_libraries(${name} PRIVATE ${ARG_LINK_LIBRARIES})
  endif()

  if (ARG_PROPERTIES)
    # E.g.
    # FOLDER;Tests;LABELS;Example;unittest;example_label;TEST_SUITE;ExampleTests
    message(SEND_ERROR "${ARG_PROPERTIES}")
    set_target_properties(${target_name} PROPERTIES ${ARG_PROPERTIES})

    # NOTE: here is a good place to collect list of supported
    # properties, and parsed values, and remove them from
    # ARG_PROPERTIES, to forward any remaining unparsed content
    # through to built-in command ``set_target_properties``.

    if( ${ARG_UNPARSED_ARGUMENTS} )
      message(WARNING "add_emoota_executable: Unparsed Arguments: ${ARG_UNPARSED_ARGUMENTS}")
    endif()

  endif()
endfunction()

function(_extend_emoota_test test_name)
  get_property(has_type
    TEST ${test_name}
    PROPERTY TEST_TYPE SET )

  if( NOT has_type )
    #send_error?
    #message?
    message(SEND_ERROR "Sink the ship")
    return()
  endif()

  get_property(test_type
    TEST ${test_name}
    PROPERTY TEST_TYPE )
  if("unknown" STREQUAL "${test_type}")
    #send_error?
    #message?
    cmake_print_properties(TESTS ${test_name} PROPERTIES TEST_TYPE  )
    message(SEND_ERROR "Sink the ship")
    return()
  endif()

  include(emoota-${test_type})
  _extend_test(${test_name})

endfunction()

#[========================================[.rst:
_emoota_register_test
________________

.. code-block:: cmake

_emoota_register_test(<suite> <exe-target>)
Internal helper function used by `add_emoota_unittest()`.
This function registers a test with CTest(1) by wrapping `add_test()` with some
common boilerplate to set test properties. The new `test` is labeled as belonging
to the the suite `<suite>` by appending the name of `<suite> to the LABELS test property.
#]========================================]

function(_emoota_register_test suite target )

  # Basic search ordering:
  # 1. Check if properties are set indicating type
  # 2. Fallback to looking through LINK_LIBRARIES property for "test" target linkage.

  message(STATUS "Registering test: ${target}")
  message(STATUS "${ARGN}")

  cmake_parse_arguments("" "" "WORKING_DIRECTORY" "PROPERTIES" ${ARGN})
  if (_WORKING_DIRECTORY)
    set(workdir "${_WORKING_DIRECTORY}")
  else()
    set(workdir "${CMAKE_CURRENT_BINARY_DIR}")
  endif()

  #unconditially update label list to include name of test suite.
  set_property(TARGET ${target} APPEND PROPERTY LABELS ${suite})

  get_property(
    has_type
    TARGET ${target}
    PROPERTY TEST_TYPE
    SET
  )
  #message(STATUS "_emoota_register_test: Checking ${target} has PROPERTY TEST_TYPE set: ${has_type}.")
  set(test_type unknown)
  if( has_type )
    get_property(
      test_type
      TARGET ${target}
      PROPERTY TEST_TYPE )
  endif()

  if ( ${test_type} STREQUAL "unknown")

    get_property(linkage
      TARGET ${target}
      PROPERTY LINK_LIBRARIES )

    foreach(link ${linkage})
      message(STATUS "_emoota_register_test: Checking for supported test library: ${link}")
      #test supported,
      if (${link} IN_LIST EMOOTA_SUPPORTED_TEST_LIBS)
        message(STATUS "_emoota_register_test: Found test to use supported test library: ${link}")
        #when supported, break;
        set(test_type ${link})
        break()
      endif()
      #else continue;
    endforeach()

    set_property(TARGET ${target} PROPERTY TEST_TYPE ${test_type})

  endif()


  # Below is a two-step process because cmakes' configure_file() command does not
  # handle "Generator Expressions", yet file(GENERATE ...) does.
  set(IFS " ")
  set(test_cmd)
  set(test_archive)
  if (EMOOTA_TEST_COVERAGE)
    set(_EMOOTA_COLLECT_CODE_COVERAGE 1)
  else()
    set(_EMOOTA_COLLECT_CODE_COVERAGE 0)
  endif()

  set(test_cmd "$<TARGET_FILE:${target}> $<JOIN:$<TARGET_PROPERTY:${target},TEST_ARGS>,${IFS}>")
  get_filename_component(test_archive "${EMOOTA_TEST_COVERAGE_DIR}/${test_name}_coverage.tgz" REALPATH)
  configure_file(${_EMOOTA_TEST_DRIVER_TEMPLATE} ${CMAKE_CURRENT_BINARY_DIR}/${test_name}.sh.in @ONLY)
  file(GENERATE
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${test_name}.sh
    INPUT ${CMAKE_CURRENT_BINARY_DIR}/${test_name}.sh.in)

  add_test(NAME ${target}
    COMMAND bash ${CMAKE_CURRENT_BINARY_DIR}/${test_name}.sh
    WORKING_DIRECTORY ${workdir})
  unset(IFS)

  get_property(target_has_labels
    TARGET ${target}
    PROPERTY LABELS
    SET
  )
  if(target_has_labels)
    get_property(target_labels TARGET ${target} PROPERTY LABELS )
    message(STATUS "Setting test labels based on <Target>: ${target} labels ${target_labels}")
    set_tests_properties(${test_name} PROPERTIES LABELS "${target_labels}")
  endif()

  if (NOT (${test_type} MATCHES "unknown|gtest|cmocka"))
    message(WARNING "Unsupported test type.")
  else()
  endif()

  set_tests_properties(${test_name} PROPERTIES TEST_TYPE "${test_type}")
  if (_PROPERTIES)
    list(LENGTH _PROPERTIES prop_len)
    if( prop_len GREATER 0 )
      set_tests_properties(${test_name} PROPERTIES ${_PROPERTIES})
    endif()
  endif()
  if (_UNPARSED_ARGUMENTS)
    message(SEND_ERROR "Unparsed arguments ${_UNPARSED_ARGUMENTS})")
  endif()

  #cmake_print_properties(TESTS ${test_name} PROPERTIES LABELS TEST_TYPE  )

endfunction()

#[========================================[.rst:
# _pull_apart_test_env_binding
# ___________________

# .. code-block:: cmake

#   _pull_apart_test_env_binding(<out-var> <out-val> <binding>)

# Synopsis
# ^^^^^^^^
# When <binding> can not be treated as a 'VAR=VAL' environment binding
# both <out-var> and <out-val> will be set to NOT_FOUND.

# Whenever <out-var> is set to NOT_FOUND, <out-val> will always be set to
# NOT_FOUND, this is invarient across failure modes.

# Example
# ^^^^^^^

# set(var "")
# set(val "")
# set(binding "SOME_IMPORTANT_VAR=some_important_val")
# _pull_apart_test_env_binding(var val ${binding})
# message(status "Pulled apart ${var}=${val}")

#]========================================]
function(_pull_apart_test_env_binding out_var out_val binding)

  # Quick Synop:
  # ------------
  # Find first '=' within ${binding}
  # save index of first '=' into _i
  # save prefix (everything before index _i) into var
  # save everything (everthing after index _i) into val

  set(${out_var})
  set(${out_val})
  set(_var NOTFOUND)
  set(_val NOTFOUND)
  set(_i -1)
  string(FIND "${binding}" "=" _i)

  if (_i EQUAL -1)
    # pass, not found
    message(DEBUG "Unable to find literal '=' value in ${binding}")
  else()
    # only obtain length when substring found.
    string(LENGTH "${binding}" _length)

    math(EXPR _j "${_i}  + 1" OUTPUT_FORMAT DECIMAL)

    # extract prefix into _var, and suffix into _val
    string(SUBSTRING "${binding}" 0 ${_i} _var)
    string(SUBSTRING "${binding}" ${_j} ${_length} _val)

  endif()

  set(${out_var} ${_var} PARENT_SCOPE)
  set(${out_val} ${_val} PARENT_SCOPE)
  return(${_var} ${_val})

endfunction()

#[========================================[.rst:
add_emoota_unittest
________________

.. code-block:: cmake

add_emoota_unittest(<suite-name> <test-target-name>
  [DEPENDS <dep1> [dep2...]]
  [WORKING_DIRECTORY <dir>]
  [LINK_LIBRARIES <lib> [<lib2>]]
  [LABELS <label1>;[<label2>..]]
  [SOURCES <label1> [<label2>..]]
  [CMDLINE_ARGS <arg1> [arg2...]])
#]========================================]
function(add_emoota_unittest test_suite test_name)
  message(STATUS "add_emoota_unittest: ${ARGN}")
  cmake_parse_arguments(PARSE_ARGV 2 ARG #prefix
    ""                                   #options
    "WORKING_DIRECTORY"                  #oneValArg
    "DEPENDS;LINK_LIBRARIES;LABELS;SOURCES;CMDLINE_ARGS")     #multiValArg

  if (ARG_WORKING_DIRECTORY)
    set(workdir ${ARG_WORKING_DIRECTORY})
  else()
    set(workdir ${${PROJECT_NAME}_BINARY_DIR})
  endif()
  if (ARG_UNPARSED_ARGUMENTS)
    message(SEND_ERROR "${ARG_UNPARSED_ARGUMENTS}")
  endif()

  # add exe target, forwarding extra args.
  if( NOT EMOOTA_BUILD_TESTS )
    add_emoota_executable(${test_name} EXCLUDE_FROM_ALL
      ${ARG_UNPARSED_ARGUMENTS}
      DEPENDS ${ARG_DEPENDS}
      LINK_LIBRARIES ${ARG_LINK_LIBRARIES} )
  else()
    add_emoota_executable(${test_name}
      ${ARG_UNPARSED_ARGUMENTS}
      DEPENDS ${ARG_DEPENDS}
      LINK_LIBRARIES ${ARG_LINK_LIBRARIES} )
  endif()
    add_custom_target("${test_suite}.${test_name}" DEPENDS ${test_name})


  target_sources(${test_name} PRIVATE
          ${ARG_SOURCES} )

  set_property(TARGET ${test_name} PROPERTY TEST_ARGS ${ARG_CMDLINE_ARGS})

  get_property(
    has_suite
    TARGET ${test_name}
    PROPERTY TEST_SUITE
    SET
  )
  if (has_suite)
    set_property(TARGET ${test_name} APPEND PROPERTY TEST_SUITE "${test_suite}")
  else()
    set_property(TARGET ${test_name} PROPERTY TEST_SUITE "${test_suite}")
  endif()

  get_property(
    has_labels
    TARGET ${test_name}
    PROPERTY LABELS
    SET
  )
  if (has_labels)
    set_property(TARGET ${test_name} APPEND PROPERTY LABELS ${ARG_LABELS})
  else()
    set_property(TARGET ${test_name} PROPERTY LABELS ${ARG_LABELS})
  endif()

  set_property(TARGET ${test_name} PROPERTY TEST_TYPE unknown)

  # Test registration process
  # Summary: try gtest, try cmocka, raise warning.
  _emoota_register_test(${test_suite} ${test_name}
    ${ARG_UNPARSED_ARGUMENTS}
    WORKING_DIRECTORY "${workdir}" )

  _extend_emoota_test(${test_name} )

  # cmake_print_properties( TARGETS ${test_name}
  #   PROPERTIES
  #        TEST_TYPE
  #        LABELS
  #        TEST_SUITE
  #        INTERFACE_LINK_LIBRARIES
  #        LINK_LIBRARIES
  # )

  get_property(inherited_environment
    TARGET ${test_suite} PROPERTY ENVIRONMENT)

  #TODO Gaurd against not found.
  #message(AUTHOR_WARNING "TODO Guard against undefind property on <test_suite>.")

  foreach(binding ${inherited_environment} )
    # pull appart environment pair
    _pull_apart_test_env_binding(envvar envval ${binding})
    if (NOT envvar)
      message(WARNING "Bad value found in ENVIRONMENT property on <test_suite> target ${test_suite}")
      message(WARNING "offending value is ${inherited_environment}")
      message(WARNING "unable to recognize ${binding}")
      continue()
    else()
      message(STATUS "${test_name}: Appending ${envvar}=${envval} to test environment.")
      set_property(TEST ${test_name} APPEND PROPERTY ENVIRONMENT ${envvar}=${envval})
    endif()
  endforeach()
  add_dependencies(${test_suite} ${test_name})
  set_property(TARGET ${test_suite} APPEND PROPERTY REGISTERED_TESTS ${test_name})
endfunction(add_emoota_unittest)


#[========================================[.rst:
add_emoota_testsuite
_________________

.. code-block:: cmake

add_emoota_testsuite(<suite-name> [EXCLUDE_FROM_CHECK_ALL]
  [ALIAS <runner-name>]
  [DEPENDS [<dep1> [<dep2>..]]  )
#]========================================]
function(add_emoota_testsuite suite_name)
  message(STATUS "add_emoota_testsuite: ${ARGN}")

  cmake_parse_arguments(ARG
    "EXCLUDE_FROM_CHECK_ALL"
    "ALIAS"
    "DEPENDS" ${ARGN} )

  add_custom_target(${suite_name}
    ${ARG_UNPARSED_ARGUMENTS}
    WORKING_DIRECTORY ${${PROJECT_NAME}_BINARY_DIR}  #Is this the correct choice?
    DEPENDS ${ARG_DEPENDS}  )

  set_target_properties(${suite_name} PROPERTIES FOLDER "Tests")
  set_property(GLOBAL APPEND PROPERTY EMOOTA_TESTSUITE ${suite_name})

  if(ARG_ALIAS)
    if (NOT DEFINED CMAKE_CTEST_COMMAND)
      message(SEND_ERROR "Please check CMAKE_CTEST_COMMAND is defined and has correct value: ${CMAKE_CTEST_COMMAND}.")
    endif()
    add_custom_target(${ARG_ALIAS}
      COMMAND ${CMAKE_CTEST_COMMAND} -L "${suite_name}" -VV
      DEPENDS ${suite_name}
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} )
    set_target_properties(${ARG_ALIAS} PROPERTIES FOLDER "Tests")

    if( NOT ARG_EXCLUDE_FROM_CHECK_ALL )
      # Register the testsuite with the global check rule.
      set_property(GLOBAL APPEND PROPERTY EMOOTA_TESTSUITE_RUNNERS ${ARG_ALIAS})
    endif()
  endif()

  set(TESTS "") #empty list
  set(TESTS_EXTRA_ENVIRONMENT "") #empty list
  set_target_properties(${suite_name} PROPERTIES
    REGISTERED_TESTS "${TESTS}"
    ENVIRONMENT "${TESTS_EXTRA_ENVIRONMENT}"
    )

endfunction()

# Restore project's policies
cmake_policy(POP)
