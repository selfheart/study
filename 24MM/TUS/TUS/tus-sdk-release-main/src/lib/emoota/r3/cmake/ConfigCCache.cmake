
# Build ref-app with ccache if the package is present
set(EMOOTA_CCACHE_BUILD OFF CACHE BOOL "Set to ON for a ccache enabled build")

if(EMOOTA_CCACHE_BUILD)
  find_program(CCACHE_PROGRAM ccache)
  if(CCACHE_PROGRAM)

    set(EMOOTA_CCACHE_MAXSIZE "" CACHE STRING "Size of ccache")
    set(EMOOTA_CCACHE_DIR "" CACHE STRING "Directory to keep ccached data")
    set(EMOOTA_CCACHE_PARAMS "CCACHE_CPP2=yes CCACHE_HASHDIR=yes" CACHE STRING "Parameters to pass through to ccache")
    set(CCACHE_PROGRAM "${EMOOTA_CCACHE_PARAMS} ${CCACHE_PROGRAM}")
    if (EMOOTA_CCACHE_MAXSIZE)
      set(CCACHE_PROGRAM "CCACHE_MAXSIZE=${EMOOTA_CCACHE_MAXSIZE} ${CCACHE_PROGRAM}")
    endif()
    if (EMOOTA_CCACHE_DIR)
      set(CCACHE_PROGRAM "CCACHE_DIR=${EMOOTA_CCACHE_DIR} ${CCACHE_PROGRAM}")
    endif()
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${CCACHE_PROGRAM})

  else()
    message(FATAL_ERROR "Unable to find the program ccache. Set EMOOTA_CCACHE_BUILD to OFF")
  endif()
endif()
