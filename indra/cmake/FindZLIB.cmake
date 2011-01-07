# -*- cmake -*-

# - Find zlib
# Find the ZLIB includes and library
# This module defines
#  ZLIB_INCLUDE_DIRS, where to find zlib.h, etc.
#  ZLIB_LIBRARIES, the libraries needed to use zlib.
#  ZLIB_FOUND, If false, do not try to use zlib.
#
# This FindZLIB is about 43 times as fast the one provided with cmake (2.8.x),
# because it doesn't look up the version of zlib, resulting in a dramatic
# speed up for configure (from 4 minutes 22 seconds to 6 seconds).
#
# Note: Since this file is only used for standalone, the windows
# specific parts were left out.

FIND_PATH(ZLIB_INCLUDE_DIR zlib.h
  NO_SYSTEM_ENVIRONMENT_PATH
  )

FIND_LIBRARY(ZLIB_LIBRARY z)

if (ZLIB_LIBRARY AND ZLIB_INCLUDE_DIR)
  SET(ZLIB_INCLUDE_DIRS ${ZLIB_INCLUDE_DIR})
  SET(ZLIB_LIBRARIES ${ZLIB_LIBRARY})
  SET(ZLIB_FOUND "YES")
else (ZLIB_LIBRARY AND ZLIB_INCLUDE_DIR)
  SET(ZLIB_FOUND "NO")
endif (ZLIB_LIBRARY AND ZLIB_INCLUDE_DIR)

if (ZLIB_FOUND)
  if (NOT ZLIB_FIND_QUIETLY)
	message(STATUS "Found ZLIB: ${ZLIB_LIBRARIES}")
	SET(ZLIB_FIND_QUIETLY TRUE)
  endif (NOT ZLIB_FIND_QUIETLY)
else (ZLIB_FOUND)
  if (ZLIB_FIND_REQUIRED)
	message(FATAL_ERROR "Could not find ZLIB library")
  endif (ZLIB_FIND_REQUIRED)
endif (ZLIB_FOUND)

mark_as_advanced(
  ZLIB_LIBRARY
  ZLIB_INCLUDE_DIR
  )

