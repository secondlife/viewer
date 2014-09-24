# -*- cmake -*-

# - Find uriparser
# Find the URIPARSER includes and library
# This module defines
#  URIPARSER_INCLUDE_DIRS, where to find uriparser.h, etc.
#  URIPARSER_LIBRARIES, the libraries needed to use uriparser.
#  URIPARSER_FOUND, If false, do not try to use uriparser.
#
# This FindURIPARSER is about 43 times as fast the one provided with cmake (2.8.x),
# because it doesn't look up the version of uriparser, resulting in a dramatic
# speed up for configure (from 4 minutes 22 seconds to 6 seconds).
#
# Note: Since this file is only used for standalone, the windows
# specific parts were left out.

FIND_PATH(URIPARSER_INCLUDE_DIR uriparser/uri.h
  NO_SYSTEM_ENVIRONMENT_PATH
  )

FIND_LIBRARY(URIPARSER_LIBRARY uriparser)

if (URIPARSER_LIBRARY AND URIPARSER_INCLUDE_DIR)
  SET(URIPARSER_INCLUDE_DIRS ${URIPARSER_INCLUDE_DIR})
  SET(URIPARSER_LIBRARIES ${URIPARSER_LIBRARY})
  SET(URIPARSER_FOUND "YES")
else (URIPARSER_LIBRARY AND URIPARSER_INCLUDE_DIR)
  SET(URIPARSER_FOUND "NO")
endif (URIPARSER_LIBRARY AND URIPARSER_INCLUDE_DIR)

if (URIPARSER_FOUND)
  if (NOT URIPARSER_FIND_QUIETLY)
    message(STATUS "Found URIPARSER: ${URIPARSER_LIBRARIES}")
    SET(URIPARSER_FIND_QUIETLY TRUE)
  endif (NOT URIPARSER_FIND_QUIETLY)
else (URIPARSER_FOUND)
  if (URIPARSER_FIND_REQUIRED)
    message(FATAL_ERROR "Could not find URIPARSER library")
  endif (URIPARSER_FIND_REQUIRED)
endif (URIPARSER_FOUND)

mark_as_advanced(
  URIPARSER_LIBRARY
  URIPARSER_INCLUDE_DIR
  )

