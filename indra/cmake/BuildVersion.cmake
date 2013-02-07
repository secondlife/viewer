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
           message("Revision (from environment): ${VIEWER_VERSION_REVISION}")

        else (DEFINED ENV{revision})
           find_program(MERCURIAL hg)
           if (DEFINED MERCURIAL)
              execute_process(
                 COMMAND ${MERCURIAL} parents --template "{rev}"
                 OUTPUT_VARIABLE VIEWER_VERSION_REVISION
                 OUTPUT_STRIP_TRAILING_WHITESPACE
                 )
              if (DEFINED VIEWER_VERSION_REVISION)
                 message("Revision (from hg) ${VIEWER_VERSION_REVISION}")
              else (DEFINED VIEWER_VERSION_REVISION)
                 set(VIEWER_VERSION_REVISION 0 )
                 message("Revision not set, repository not found, using ${VIEWER_VERSION_REVISION}")
              endif (DEFINED VIEWER_VERSION_REVISION)
           else (DEFINED MERCURIAL)
              set(VIEWER_VERSION_REVISION 0)
              message("Revision not set, 'hg' not found (${MERCURIAL}), using ${VIEWER_VERSION_REVISION}")
           endif (DEFINED MERCURIAL)
        endif (DEFINED ENV{revision})
        message("Building Version ${VIEWER_SHORT_VERSION} ${VIEWER_VERSION_REVISION}")
    else ( EXISTS ${VIEWER_VERSION_BASE_FILE} )
        message(SEND_ERROR "Cannot get viewer version from '${VIEWER_VERSION_BASE_FILE}'") 
    endif ( EXISTS ${VIEWER_VERSION_BASE_FILE} )

    set(VIEWER_CHANNEL_VERSION_DEFINES
        "LL_VIEWER_CHANNEL=\"${VIEWER_CHANNEL}\""
        "LL_VIEWER_VERSION_MAJOR=${VIEWER_VERSION_MAJOR}"
        "LL_VIEWER_VERSION_MINOR=${VIEWER_VERSION_MINOR}"
        "LL_VIEWER_VERSION_PATCH=${VIEWER_VERSION_PATCH}"
        "LL_VIEWER_VERSION_BUILD=${VIEWER_VERSION_REVISION}"
        )
endif (NOT DEFINED VIEWER_SHORT_VERSION)
