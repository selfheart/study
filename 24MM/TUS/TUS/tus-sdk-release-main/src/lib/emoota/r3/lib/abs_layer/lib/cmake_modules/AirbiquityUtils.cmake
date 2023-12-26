###############################################################################
 #
 #         AIRBIQUITY INC. PROPRIETARY AND CONFIDENTIAL INFORMATION
 #
 #  The software and information contained herein is the proprietary and
 #  confidential property of Airbiquity Inc. and shall not be disclosed
 #  in whole or in part. Possession, use, reproduction or transfer of
 #  this software and information is prohibited without the express written
 #  permission of Airbiquity Inc.
 #
 #  Copyright (c) 2020 Airbiquity Inc.  All rights reserved.
 #
###############################################################################
cmake_minimum_required(VERSION 3.13)


function(add_directory_property directory_name property_name property_value)
    get_property(props DIRECTORY ${directory_name} PROPERTY "${property_name}")
    list (FIND props "${property_value}" _index)
    if (0 GREATER ${_index})
        list(APPEND props "${property_value}")
        set_property (DIRECTORY ${directory_name} PROPERTY "${property_name}" ${props})
        message(STATUS "add_directory_property('${directory_name}' '${property_name}' '${property_value}')")
    endif()
endfunction()

function(set_property_globally property_name property_value)
    # message(STATUS "set_property_globally(${property_name} ${property_value})")
    set(active_dir ${CMAKE_CURRENT_SOURCE_DIR})
    while(active_dir)
        add_directory_property("${active_dir}" "${property_name}" "${property_value}")
        get_property(parent_dir
                    DIRECTORY ${active_dir}
                    PROPERTY PARENT_DIRECTORY)
        set(active_dir ${parent_dir})
    endwhile(active_dir)
endfunction()

function(add_global_compile_definition compile_definition)
    # message(STATUS "add_global_compile_definition(${compile_definition})")
    # set the COMPILE_DEFINITIONS for each directory scope
    set_property_globally(COMPILE_DEFINITIONS "${compile_definition}")
endfunction()

function(add_global_compile_options)
    #message(STATUS "add_global_compile_options(${ARGN})")
    foreach(compile_option ${ARGN})
        #message(STATUS "set_property_globally(COMPILE_OPTIONS ${compile_option})")
        # set the COMPILE_OPTIONS for each directory scope
        set_property_globally(COMPILE_OPTIONS "${compile_option}")
    endforeach()
endfunction()

function(define_filename_macro_for_sources targetname)
    #message(STATUS "define_filename_macro_for_sources(${targetname})")
    get_target_property(source_files "${targetname}" SOURCES)
    get_target_property(source_dir "${targetname}" SOURCE_DIR)
    foreach(sourcefile ${source_files})
        # Get source file's current list of compile definitions.
        get_property(defs
            SOURCE "${sourcefile}"
            PROPERTY COMPILE_DEFINITIONS)
        # Add the FILE_BASENAME=filename compile definition to the list.
        get_filename_component(basename "${source_dir}/${sourcefile}" NAME)
        list(APPEND defs "__FILENAME__=\"${basename}\"")
        list(APPEND defs "__FILE__=\"${basename}\"")
        # Set the updated compile definitions on the source file.
        # https://cmake.org/cmake/help/v3.3/command/set_source_files_properties.html
        # Due to historical development reasons, source file properties
        # are scoped only in the directory where they are set.  Only
        # targets created by an add_exectuable, add_library, or
        # add_custom_target commands in the *same* directory (not even
        # a subdirectory) can see the property of a source file.
        set_property (
            SOURCE "${sourcefile}"
            PROPERTY COMPILE_DEFINITIONS ${defs})
        #message(STATUS "${sourcefile} ${defs}")
        
        get_property(options
            SOURCE "${sourcefile}"
            PROPERTY COMPILE_OPTIONS)
        list (FIND options "-Wno-builtin-macro-redefined" _index)
        if (0 GREATER ${_index})
            list(APPEND options "-Wno-builtin-macro-redefined")
            set_property (SOURCE ${sourcefile} PROPERTY COMPILE_OPTIONS ${options})
            #message(STATUS "add_source_property('${sourcefile}' 'COMPILE_OPTIONS' '${options}')")
        endif()
    endforeach()
endfunction()

function(import_sdm_c_module targetname git_tag)
    string(TOLOWER "${targetname}" lcName)
    if(IS_DIRECTORY ${tmp_dir})
        # tmp_dir was passed in from project config
    elseif(win32)
        string(REPLACE "\\" "/" tmp_dir "$ENV{TMP}")
    else() # linux?
        set(tmp_dir "$ENV{TMPDIR}")
        if(NOT EXISTS "${tmp_dir}")
            #default to "/tmp" if we can't read from environment
            set(tmp_dir "/tmp")
        endif()
    endif()
    # populate data external to src/build folders & within the /tmp folder

    get_filename_component(FETCH_SOURCE_DIR "${tmp_dir}/sdm_c_modules/${targetname}/${git_tag}" REALPATH)
    get_filename_component(FETCH_SUBBUILD_DIR "${tmp_dir}/sdm_c_modules/subbuild/${targetname}/${git_tag}" REALPATH)
    Message(WARNING "Import dependency: ${lcName}, pulling into: ${FETCH_SOURCE_DIR}")
    file(MAKE_DIRECTORY ${FETCH_SOURCE_DIR})
    include(FetchContent)
    # Fetch the content using previously declared details
    FetchContent_Populate(${lcName} QUIET
            GIT_REPOSITORY "git@vc1.airbiquity.com:sdm_c_modules/${targetname}.git"
            GIT_TAG        "${git_tag}"
            SOURCE_DIR     "${FETCH_SOURCE_DIR}"
            SUBBUILD_DIR   "${FETCH_SUBBUILD_DIR}"
        )
    # Bring the populated content into the build
    add_subdirectory(${${lcName}_SOURCE_DIR} ${${lcName}_BINARY_DIR})
endfunction()

function(abq_path_join output_var left_path right_path)
    set(compound_path "${left_path}")
    if((right_path) AND (NOT ("${right_path}" STREQUAL "")))
        if((NOT compound_path) OR (("${compound_path}" STREQUAL "")))
            # Overwrite the empty path
            set(compound_path "${right_path}")
        elseif(IS_ABSOLUTE "${right_path}")
            # Assume right_path starts with a delimiter
            set(compound_path "${compound_path}${right_path}")
        else()
            set(compound_path "${compound_path}/${right_path}")
        endif()
    endif()
    set(${output_var} "${compound_path}" PARENT_SCOPE)
endfunction()

function(abq_ctest_add name target)
    include(CTest)
    get_property(OUTPUT_DIR TARGET ${target} PROPERTY RUNTIME_OUTPUT_DIRECTORY)
    get_property(FILENAME TARGET ${target} PROPERTY NAME)
    abq_path_join(TARGET_EXEC "${OUTPUT_DIR}" "${FILENAME}")
    # TODO check that test doesn't get added twice
    add_test(NAME "${name}"
        COMMAND "${TARGET_EXEC}"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endfunction()

cmake_policy(SET CMP0057 NEW)
# Find all targets linked to into the library
function(get_subtgt_recursively alltgtslist roottgt)
  if (TARGET ${roottgt} AND NOT ${roottgt} IN_LIST ${alltgtslist}) # a target has not been checked yet
      list(APPEND ${alltgtslist} ${roottgt})
      get_property(tgts TARGET ${roottgt} PROPERTY LINK_LIBRARIES)
      foreach(tgt IN ITEMS ${tgts})
        get_subtgt_recursively(${alltgtslist} ${tgt})
      endforeach()
  endif()
  set(${alltgtslist} ${${alltgtslist}} PARENT_SCOPE)
endfunction()

# usage: extra_files_in_code_hash(<targetname> <list of files>)
function(extra_files_in_code_hash)
  set(targetname ${ARGV0})
  set(filelist "")
  foreach (i RANGE 1 ${ARGC})
    list(APPEND filelist ${ARGV${i}})
  endforeach()

  set_property(TARGET ${targetname} PROPERTY AbqExtraFilesToHash ${filelist})
endfunction()

# usage: exclude_files_from_code_hash(<targetname> <list of files>)
function(exclude_files_from_code_hash)
  set(targetname ${ARGV0})
  set(filelist "")
  foreach (i RANGE 1 ${ARGC})
    list(APPEND filelist ${ARGV${i}})
  endforeach()

  set_property(TARGET ${targetname} PROPERTY AbqFilesNOTHashed ${filelist})
endfunction()


function(add_fingerprint_target _TARGET_NAME _VARNAME)
  #TODO: Make _VARNAME optional since we are alwasy setting propert on target.
  if(NOT TARGET ${_TARGET_NAME})
    message(FATAL_ERROR "No CMake target named ${_TARGET_NAME}")
  endif()

  if(NOT EXISTS ${FINGERPRINT_SCRIPT})
    message(FATAL_ERROR "Cant find ${FINGERPRINT_SCRIPT}")
  endif()

  set(alltgts "")
  get_subtgt_recursively(alltgts ${_TARGET_NAME})
  message(WARNING "Found dependences of ${_TARGET_NAME} to be: ${alltgts}")
  # Get list of all source code, plus ExtraFilesToHash, except for the
  # FilesNotHashed specified by target
  set(ALL_SRC "")
  foreach(tgt IN ITEMS ${alltgts})
    get_property(srcs TARGET ${tgt} PROPERTY SOURCES)
    #message(STATUS "<target> ${tgt} has sources: ${srcs}")
    get_property(extra_srcs TARGET ${tgt} PROPERTY AbqExtraFilesToHash)
    #message(STATUS "<target> ${tgt} has extra sources: ${extra_srcs}")
    get_property(srcdir TARGET ${tgt} PROPERTY SOURCE_DIR)
    #message(STATUS "<target> ${tgt} has sources directory ${srcdir}")
    get_property(notHashed TARGET ${tgt} PROPERTY AbqFilesNOTHashed)
    #message(STATUS "<target> ${tgt} excludes so urces ${notHashed}")

    foreach(src IN ITEMS ${srcs} ${extra_srcs})
      if(NOT ${src} IN_LIST notHashed)
        if(NOT IS_ABSOLUTE ${src})
          #TODO Use get_file_component() here?
          list(APPEND ALL_SRC "${srcdir}/${src}")
        else()
          list(APPEND ALL_SRC ${src})
        endif()
      endif()
    endforeach()
  endforeach()

  # Normalize the source file list
  list(REMOVE_DUPLICATES ALL_SRC)
  list(SORT ALL_SRC)

  set_target_properties(${_TARGET_NAME}
                      PROPERTIES fingerprint_files "${ALL_SRC}")

  #string(GENEX_STRIP <input string> <output variable>)
  string(GENEX_STRIP "${ALL_SRC}" ALL_SRC)
  #Return to parent
  set(${_VARNAME} ${ALL_SRC} PARENT_SCOPE)

  message(STATUS "Adding <target>: ${_TARGET_NAME}.fingerprint")
  add_custom_target(${_TARGET_NAME}.fingerprint
    COMMAND_EXPAND_LISTS
    COMMAND ${CMAKE_COMMAND} -DTARGET="${CODEHASH_H}" -DHASH_SET="${ALL_SRC}" -P ${FINGERPRINT_SCRIPT}
    BYPRODUCTS
        "${CODEHASH_H}"
    WORKING_DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS
        ${SDK_FINGERPRINT_FILES}
    COMMENT "Generating source fingerprint ${CODEHASH_H}."
    )
endfunction()


# TODO: Thu Apr  2 17:35:58 PDT 2020
# locate and drop t odisk archive_and_remove_gcda_script when not present.
function(abq_prep_for_coverage _targetname _testtarget _outputname)
  if (NOT COMMAND SETUP_TARGET_FOR_COVERAGE)
    include(CodeCoverage)
  endif()

  if(NOT DEFINED ABQ_archive_and_remove_gcda)
    message(SEND_ERROR "Define and set ABQ_archive_and_remove_gcda to path script.")
  endif()

  set(LCOV_REMOVE_EXTRA
    '$<TARGET_PROPERTY:${_testtarget},SOURCE_DIR>/test*'
    '$<TARGET_PROPERTY:cmocka,SOURCE_DIR>/*'
  )
  setup_target_for_coverage(${_targetname} ${_testtarget} ${_outputname})

  #assert _targetname is now defined and a target.
  if (NOT TARGET ${_targetname})
    message(FATAL_ERROR "Somehow ${_targetname} has not been defined.")
  endif()

  set(_byproducts "${_outputname}.profile_data.tgz")
  add_custom_command(TARGET ${_targetname} POST_BUILD
    COMMAND ${ABQ_archive_and_remove_gcda} ${CMAKE_BINARY_DIR} ${_byproducts}
     BYPRODUCTS ${_byproducts}
     COMMENT "Collecting .gcda profiling data files, and archiving into ${_byproducts}"
  )
endfunction(abq_prep_for_coverage)
