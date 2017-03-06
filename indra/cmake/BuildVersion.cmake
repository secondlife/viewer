# -*- cmake -*-
# Construct the viewer version number based on the indra/VIEWER_VERSION file

if (NOT DEFINED VIEWER_SHORT_VERSION) # will be true in indra/, false in indra/newview/
    set(VIEWER_VERSION_BASE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/newview/VIEWER_VERSION.txt")

    if ( EXISTS ${VIEWER_VERSION_BASE_FILE} )
        file(STRINGS ${VIEWER_VERSION_BASE_FILE} VIEWER_SHORT_VERSION REGEX "^[0-9]+\\.[0-9]+\\.[0-9]+")
        string(REGEX REPLACE "^([0-9]+)\\.[0-9]+\\.[0-9]+" "\\1" VIEWER_VERSION_MAJOR ${VIEWER_SHORT_VERSION})
        string(REGEX REPLACE "^[0-9]+\\.([0-9]+)\\.[0-9]+" "\\1" VIEWER_VERSION_MINOR ${VIEWER_SHORT_VERSION})
        string(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+)" "\\1" VIEWER_VERSION_PATCH ${VIEWER_SHORT_VERSION})

        if (DEFINED ENV{revision})
           set(VIEWER_VERSION_REVISION $ENV{revision})
           message(STATUS "Revision (from environment): ${VIEWER_VERSION_REVISION}")

        elseif (DEFINED ENV{AUTOBUILD_BUILD_ID})
           set(VIEWER_VERSION_REVISION $ENV{AUTOBUILD_BUILD_ID})
           message(STATUS "Revision (from autobuild environment): ${VIEWER_VERSION_REVISION}")

        else (DEFINED ENV{revision})
          find_program(MERCURIAL
                       NAMES hg
                       PATHS [HKEY_LOCAL_MACHINE\\Software\\TortoiseHG]
                       PATH_SUFFIXES Mercurial)
          mark_as_advanced(MERCURIAL)
          if (MERCURIAL)
            execute_process(COMMAND ${MERCURIAL} identify --num --rev tip
                            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                            RESULT_VARIABLE hg_id_result
                            ERROR_VARIABLE hg_id_error
                            OUTPUT_VARIABLE VIEWER_VERSION_REVISION
                            OUTPUT_STRIP_TRAILING_WHITESPACE)
            if (NOT ${hg_id_result} EQUAL 0)
              message(SEND_ERROR "Revision number generation failed with output:\n${hg_id_error}")
            else (NOT ${hg_id_result} EQUAL 0)
              string(REGEX REPLACE "[^0-9a-f]" "" VIEWER_VERSION_REVISION ${VIEWER_VERSION_REVISION})
            endif (NOT ${hg_id_result} EQUAL 0)
            if ("${VIEWER_VERSION_REVISION}" MATCHES "^[0-9]+$")
              message(STATUS "Revision (from hg) ${VIEWER_VERSION_REVISION}")
            else ("${VIEWER_VERSION_REVISION}" MATCHES "^[0-9]+$")
              message(STATUS "Revision not set (repository not found?); using 0")
              set(VIEWER_VERSION_REVISION 0 )
            endif ("${VIEWER_VERSION_REVISION}" MATCHES "^[0-9]+$")
           else (MERCURIAL)
              message(STATUS "Revision not set: mercurial not found; using 0")
              set(VIEWER_VERSION_REVISION 0)
           endif (MERCURIAL)
        endif (DEFINED ENV{revision})
        message(STATUS "Building '${VIEWER_CHANNEL}' Version ${VIEWER_SHORT_VERSION}.${VIEWER_VERSION_REVISION}")
    else ( EXISTS ${VIEWER_VERSION_BASE_FILE} )
        message(SEND_ERROR "Cannot get viewer version from '${VIEWER_VERSION_BASE_FILE}'") 
    endif ( EXISTS ${VIEWER_VERSION_BASE_FILE} )

    if ("${VIEWER_VERSION_REVISION}" STREQUAL "")
      message(STATUS "Ultimate fallback, revision was blank or not set: will use 0")
      set(VIEWER_VERSION_REVISION 0)
    endif ("${VIEWER_VERSION_REVISION}" STREQUAL "")

    set(VIEWER_CHANNEL_VERSION_DEFINES
        "LL_VIEWER_CHANNEL=${VIEWER_CHANNEL}"
        "LL_VIEWER_VERSION_MAJOR=${VIEWER_VERSION_MAJOR}"
        "LL_VIEWER_VERSION_MINOR=${VIEWER_VERSION_MINOR}"
        "LL_VIEWER_VERSION_PATCH=${VIEWER_VERSION_PATCH}"
        "LL_VIEWER_VERSION_BUILD=${VIEWER_VERSION_REVISION}"
        "LLBUILD_CONFIG=\"${CMAKE_BUILD_TYPE}\""
        )
endif (NOT DEFINED VIEWER_SHORT_VERSION)
