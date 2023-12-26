
function(_extend_test test)

  if ((NOT ${EMOOTA_TEST_RECORDS}) OR
      (NOT TEST ${test}))
    #send-error?
    #message?
    return()
  endif()

#  CMOCKA_MESSAGE_OUTPUT="XML"
#  CMOCKA_XML_FILE=<EMOOTA_TEST_RECORDS_DIR>/$<target_name>.xml
  set(cmocka_envlist )
  set(cmocka_output_format "")
  set(cmocka_output_destination "")

  if (DEFINED EMOOTA_TEST_RECORD_FORMAT)
    string(TOLOWER "${EMOOTA_TEST_RECORD_FORMAT}" lowercase_EMOOTA_TEST_RECORD_FORMAT)

    cmake_print_variables(EMOOTA_TEST_RECORD_FORMAT lowercase_EMOOTA_TEST_RECORD_FORMAT)

    if ("${lowercase_EMOOTA_TEST_RECORD_FORMAT}" STREQUAL "none")
      message(AUTHOR_WARNING "_extend_test: Leaving early")
      return()
    endif()

    set (cmocka_output_format ${lowercase_EMOOTA_TEST_RECORD_FORMAT})
    list(APPEND cmocka_envlist "CMOCKA_MESSAGE_OUTPUT=${lowercase_EMOOTA_TEST_RECORD_FORMAT}")
  endif()


  if (DEFINED EMOOTA_TEST_RECORDS_DIR)
    get_filename_component(output_destination "${EMOOTA_TEST_RECORDS_DIR}/${test}" REALPATH)
    set(cmocka_output_destination "${output_destination}.${cmocka_output_format}")
    list(APPEND cmocka_envlist "CMOCKA_XML_FILE=${cmocka_output_destination}")
  endif()

  get_property(has_env
    TEST ${test}
    PROPERTY ENVIRONMENT SET )
  if(has_env)
    set_property(TEST ${test} PROPERTY ENVIRONMENT APPEND ${cmocka_envlist} )
  else()
    set_property(TEST ${test} PROPERTY ENVIRONMENT ${cmocka_envlist} )
  endif()


endfunction()
