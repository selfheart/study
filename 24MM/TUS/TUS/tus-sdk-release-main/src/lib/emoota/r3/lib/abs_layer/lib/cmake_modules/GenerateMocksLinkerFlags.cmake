#/* ****************************************************************************
# *
# *  generate_mocks_linker_flags(FLAGS mock1 ... mockN)
# *
# *  Creates linker "--wrap" flags for all mock function strings parameters.
# *  Full set of linker flags are stored in specified FLAGS parameter.
# *
# *  Example:
# *    generate_mocks_linker_flags(RESULT function1 function2)
# *    message(STATUS "RESULT=" ${RESULT})
# *
# *    Output:
# *    -- RESULT=-Wl,--wrap=function1,--wrap=function2
# *
# * ****************************************************************************
# */
function(generate_mocks_linker_flags FLAGS)
    set(mocks_linker_flags "-Wl")
    foreach(mock ${ARGN})
        set(mocks_linker_flags "${mocks_linker_flags},--wrap=${mock}")
    endforeach()
    set(${FLAGS} "${mocks_linker_flags}" PARENT_SCOPE)
endfunction(generate_mocks_linker_flags)
