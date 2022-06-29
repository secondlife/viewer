# -*- cmake -*-

# - Find zlib-ng
# Find the ZLIB includes and library
# This module defines
#  ZLIBNG_INCLUDE_DIRS, where to find zlib.h, etc.
#  ZLIBNG_LIBRARIES, the libraries needed to use zlib.
#  ZLIBNG_FOUND, If false, do not try to use zlib.
#
# This FindZLIBNG is about 43 times as fast the one provided with cmake (2.8.x),
# because it doesn't look up the version of zlib, resulting in a dramatic
# speed up for configure (from 4 minutes 22 seconds to 6 seconds).
#
# Note: Since this file is only used for standalone, the windows
# specific parts were left out.

FIND_PATH(ZLIBNG_INCLUDE_DIR zlib.h
  NO_SYSTEM_ENVIRONMENT_PATH
  )

FIND_LIBRARY(ZLIBNG_LIBRARY z)

if (ZLIBNG_LIBRARY AND ZLIBNG_INCLUDE_DIR)
  SET(ZLIBNG_INCLUDE_DIRS ${ZLIBNG_INCLUDE_DIR})
  SET(ZLIBNG_LIBRARIES ${ZLIBNG_LIBRARY})
  SET(ZLIBNG_FOUND "YES")
else (ZLIBNG_LIBRARY AND ZLIBNG_INCLUDE_DIR)
  SET(ZLIBNG_FOUND "NO")
endif (ZLINGB_LIBRARY AND ZLIBNG_INCLUDE_DIR)

if (ZLIBNG_FOUND)
  if (NOT ZLIBNG_FIND_QUIETLY)
    message(STATUS "Found ZLIBNG: ${ZLIBNG_LIBRARIES}")
    SET(ZLIBNG_FIND_QUIETLY TRUE)
  endif (NOT ZLIBNG_FIND_QUIETLY)
else (ZLIBNG_FOUND)
  if (ZLIBNG_FIND_REQUIRED)
    message(FATAL_ERROR "Could not find ZLIBNG library")
  endif (ZLIBNG_FIND_REQUIRED)
endif (ZLIBNG_FOUND)

mark_as_advanced(
  ZLIBNG_LIBRARY
  ZLIBNG_INCLUDE_DIR
  )

