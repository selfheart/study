# -*- mode: cmake; -*-
cmake_minimum_required(VERSION 3.13)
# File: utility/cmake/CTestCoverageCollectLCOV.cmake
#[=======================================================================[.rst:

          AIRBIQUITY INC. PROPRIETARY AND CONFIDENTIAL INFORMATION

   The software and information contained herein is the proprietary and
   confidential property of Airbiquity Inc. and shall not be disclosed
   in whole or in part. Possession, use, reproduction or transfer of
   this software and information is prohibited without the express written
   permission of Airbiquity Inc.

   Copyright © 2021 Airbiquity Inc.  All rights reserved.


CTestCoverageCollectLCOV
------------------------

This module provides the ``ctest_coverage_collect_lcov`` function.

this function runs lcov on a test tree, and packages the resulting
files into a tar archive.  This tarball contains the following:

Testing/test_desc.txt
Testing/CoveragInfo/baseline.info
Testing/CoveragInfo/coverage.info


* TODO: list archive contents.

.. command:: ctest_coverage_collect_lcov(TARBALL <tarfile>
                                         [SOURCE <source_dir>]
                                         [BUILD <build_dir>]
                                         [TEST_INFO < FILE <file>
                                                    | NAME <name> DESCRIPTION <description> > ]
                                         [DELETE]

  #TODO: Consider option for supplying .lcovrc file?
  #TODO: Consider option for supplying coverage directory? Defaults to Testing/CoveragInfo/
  #TODO: Consider option for supplying lcov options directly? Default to some basics?
  #      We use `--directory=<dir> --initial --capture --output-file=<output.info>` to baseline,
  #             `--directory=<dir> --capture --output-file=<output.info>` to measure,
  #             `--append` to combine two info files.
  #             `--extract` to filter based on REGEX.
  )

  Run lcov and package a tar file. The options are:

  ``TARBALL <tarfile>``
    Specify the location of the ``.tar`` file to
    be created for later processing (maybe jenkins, maybe manual
    inspection, whatever).  Relative paths will be interpreted with
    respect first to ``BUILD <build_dir>`` and default back to
    :variable: `CTEST_BINARY_DIR`, the top-level test directory.

  ``SOURCE <source_dir>``
    Specify the top-level source directory for the build.
    Default is the value of :variable:`CTEST_SOURCE_DIRECTORY`.

  ``BUILD <build_dir>``
    Specify the top-level build directory for the build.
    Default is the value of :variable:`CTEST_BINARY_DIRECTORY`.

  ``TEST_INFO TODO``
    FIXME!

  ``DELETE``
    Delete coverage files after they've been packaged into the .tar.

#]=======================================================================]#

# TODO:
# ARG_OPTIONS seems totally unused.

#Maybe these are an aweful idea.

if (NOT COMMAND notify)
  function(notify)
    message(STATUS "${PREFIX} ${ARGN}")
  endfunction()
endif()
if (NOT COMMAND error)
  macro(error)
    message(SEND_ERROR "${PREFIX} ${ARGN}")
  endmacro()
endif()
#end maybe aweful idea.

function(ctest_coverage_collect_lcov)
  cmake_parse_arguments(ARG
    "DELETE"
    "TARBALL;SOURCE;BUILD;LCOV_COMMAND;"
    "TEST_INFO"
    ${ARGN})

  if(NOT DEFINED ARG_TARBALL)
    error("TARBALL must be specified for command: ctest_coverage_collect_lcov")
  endif()

  if(NOT DEFINED ARG_SOURCE)
    set(source_dir "${CTEST_SOURCE_DIRECTORY}")
  else()
    set(source_dir "${ARG_SOURCE}")
  endif()

  if(NOT DEFINED ARG_BUILD)
    set(build_dir "${CTEST_BINARY_DIRECTORY}")
  else()
    set(build_dir "${ARG_BUILD}")
  endif()

  # Possibly construct directory for the coverage files.
  get_filename_component(coverage_dir "${build_dir}/Testing/CoverageInfo" ABSOLUTE)
  file(MAKE_DIRECTORY "${coverage_dir}")
  set(manifest "")
  set(manifest_file "${coverage_dir}/coverage_file_list.txt")

  #can macro these two? 1/2
  if(NOT DEFINED ARG_LCOV_COMMAND)
    notify( "Locating gendesc via find_program()")
    find_program(gendesc_command gendesc)
    if ("${gendesc_command}" STREQUAL "gendesc_command-NOTFOUND")
      error("unable to locate prefered gendesc utility")
    else ()
      notify("Using gendesc: ${gendesc_command}")
    endif()
  else()
    set(gendesc_command "${ARG_GENDESC_COMMAND}")
  endif()
  # 2/2
  if(NOT DEFINED ARG_LCOV_COMMAND)
    notify( "Locating lcov via find_program()")
    find_program(lcov_command lcov)

    if ("${lcov_command}" STREQUAL "lcov_command-NOTFOUND")
      error("unable to locate prefered lcov utility")
    else ()
      notify("Using lcov: ${lcov_command}")
    endif()
  else()
    set(lcov_command "${ARG_LCOV_COMMAND}")
  endif()

  #TODO remove hardcode value
  list(APPEND ARG_OPTIONS "--gcov-tool;gcov-11")
  list(APPEND ARG_OPTIONS "--rc;lcov_branch_coverage=1")

  if(DEFINED ARG_TEST_INFO)
    cmake_parse_arguments("TINFO"
      ""
      "NAME;DESCRIPTION;FILE"
      ""
      ${ARG_TEST_INFO})
    if (((DEFINED TINFO_NAME) OR (DEFINED TINFO_DESCRIPTION)) AND
        (DEFINED TINFO_FILE))
      error("Use one or the other. not both FIXME error comment")
    endif()

    #handle case when no info file,
    if (NOT DEFINED TINFO_FILE)
      #create info file, and treat as if given info file.
      get_filename_component( test_info_dir "${coverage_dir}" DIRECTORY )
      get_filename_component( test_info_dir "${test_info_dir}" ABSOLUTE )
      get_filename_component( test_info_fname "${test_info_dir}/${TINFO_NAME}.txt" ABSOLUTE )
      file(WRITE ${test_info_fname} "${TINFO_NAME}")
      file(APPEND ${test_info_fname} "\n")
      file(APPEND ${test_info_fname} " ") #importantn
      file(APPEND ${test_info_fname} "${TINFO_DESCRIPTION}")
      set(TINFO_FILE ${test_info_fname})
    endif()

    set(test_info "${TINFO_FILE}")
    get_filename_component( test_info_dir "${coverage_dir}" DIRECTORY )
    get_filename_component( test_info_dir "${test_info_dir}" ABSOLUTE )
    get_filename_component( test_info_fname "${TINFO_FILE}" NAME )
    file(COPY "${TINFO_FILE}" DESTINATION "${test_info_dir}")
    set(test_info_file "${test_info_dir}/${test_info_fname}")

    get_filename_component(test_desc_file "${coverage_dir}/../test_desc.txt" ABSOLUTE)
    execute_process(COMMAND
      ${gendesc_command} "${test_info_file}"
      OUTPUT_VARIABLE out
      RESULT_VARIABLE res
      OUTPUT_FILE "${test_desc_file}"
      COMMAND_ECHO "STDERR"
      WORKING_DIRECTORY ${coverage_dir})

    if(NOT "${res}" EQUAL 0)
      notify("${gendesc_command} ${test_info_file}")
      error("Error running gendesc: ${res} ${out}")
      return()
    endif()

    file(STRINGS "${test_desc_file}" test_info)
    list(GET test_info 0 test_name)
    string(REPLACE "TN: " "" test_name ${test_name})
    list(APPEND manifest "${test_desc_file}")

  endif()

  # run lcov to obtain baseline.
  get_filename_component(info_file "${coverage_dir}/baseline.info" ABSOLUTE)
  execute_process(COMMAND
    ${lcov_command} ${ARG_OPTIONS} --directory=${build_dir} --initial --capture --output-file=${info_file}
    OUTPUT_VARIABLE out
    RESULT_VARIABLE res
    COMMAND_ECHO "STDERR"
    WORKING_DIRECTORY ${coverage_dir})
  if(NOT "${res}" EQUAL 0)
    notify("${lcov_command} ${ARG_OPTIONS} --directory=${build_dir} --initial --capture --output-file=${info_file}")
    error("Error running lcov: ${res} ${out}")
    return()
  endif()
  list(APPEND manifest "${info_file}")

  # run lcov to obtain measure.
  if(DEFINED test_name)
    list(APPEND ARG_OPTIONS "--test-name=${test_name}")
  endif()
  get_filename_component(info_file "${coverage_dir}/coverage.info" ABSOLUTE)
  execute_process(COMMAND
    ${lcov_command} ${ARG_OPTIONS} --directory=${build_dir} --capture --output-file=${info_file}
    OUTPUT_VARIABLE out
    RESULT_VARIABLE res
    COMMAND_ECHO "STDERR"
    WORKING_DIRECTORY ${coverage_dir})
  if(NOT "${res}" EQUAL 0)
    notify("${lcov_command} ${ARG_OPTIONS} --directory=${build_dir} --capture --output-file=${info_file}")
    error("Error running lcov: ${res} ${out}")
    return()
  endif()
  list(APPEND manifest "${info_file}")

  # Use lcov to produce textual human readable report.
  get_filename_component(info_file "${coverage_dir}/coverage.info" ABSOLUTE)
  execute_process(COMMAND ${lcov_command} -l ${info_file}
    OUTPUT_VARIABLE out
    RESULT_VARIABLE res
    OUTPUT_FILE "${info_file}.txt"
    COMMAND_ECHO "STDERR"
    WORKING_DIRECTORY ${coverage_dir})
  if(NOT "${res}" EQUAL 0)
    notify("${lcov_command} -l ${info_file}")
    error("Error running lcov: ${res} ${out}")
    return()
  endif()
  list(APPEND manifest  "${info_file}.txt")

  #write out manifest to disk, manifest used to create archive
  set(file_manifest)
  foreach(file IN ITEMS ${manifest})
    file(RELATIVE_PATH tmp_rel ${build_dir} ${file})
    list(APPEND file_manifest ${tmp_rel})
  endforeach()
  string(REPLACE ";" "\n" manifest "${file_manifest}")
  file(WRITE "${manifest_file}" "${manifest}")
  unset(manifest)

  #create archive
  set(tar_opts "cfvz")
  execute_process(COMMAND
    ${CMAKE_COMMAND} -E tar ${tar_opts} ${ARG_TARBALL}
    #    "--mtime=1970-01-01 0:0:0 UTC"
    "--format=gnutar"
    "--files-from=${manifest_file}"
    OUTPUT_VARIABLE out
    RESULT_VARIABLE res
    COMMAND_ECHO "STDERR"
    WORKING_DIRECTORY ${build_dir})

  #clean up
  if (ARG_DELETE)
    file(REMOVE ${manifest_file})
    execute_process(COMMAND
      "${lcov_command} --zerocounters --directory=${build_dir}"
      OUTPUT_VARIABLE out
      RESULT_VARIABLE res
      COMMAND_ECHO "STDERR"
      WORKING_DIRECTORY ${coverage_dir})
    if(NOT "${res}" EQUAL 0)
      notify("${lcov_command} --zerocounters --directory=${build_dir}")
      error("Error running lcov: ${res} ${out}")
      return()
    endif()
  endif()

endfunction()

#set(CTC_LCOV_PREFIX "CTestCoverageCollectLCOV")
unset(CTC_LCOV_PREFIX)
