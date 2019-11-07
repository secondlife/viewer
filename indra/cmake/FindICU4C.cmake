# -*- cmake -*-

# - Find ICU4C
# This module defines
#  ICU4C_INCLUDE_DIR, where to find headers
#  ICU4C_LIBRARY, the library needed to use ICU4C.
#  ICU4C_FOUND, If false, do not try to use ICU4C.

find_path(ICU4C_INCLUDE_DIR uchar.h
  PATH_SUFFIXES unicode
  )

set(ICU4C_NAMES ${ICU4C_NAMES} icuuc)
find_library(ICU4C_LIBRARY
  NAMES ${ICU4C_NAMES}
  )

if (ICU4C_LIBRARY AND ICU4C_INCLUDE_DIR)
  set(ICU4C_FOUND "YES")
else (ICU4C_LIBRARY AND ICU4C_INCLUDE_DIR)
  set(ICU4C_FOUND "NO")
endif (ICU4C_LIBRARY AND ICU4C_INCLUDE_DIR)

if (ICU4C_FOUND)
    message(STATUS "Found ICU4C: Library in '${ICU4C_LIBRARY}' and header in '${ICU4C_INCLUDE_DIR}' ")
else (ICU4C_FOUND)
    message(FATAL_ERROR " * * *\nCould not find ICU4C library! * * *")
endif (ICU4C_FOUND)

mark_as_advanced(
  ICU4C_LIBRARY
  ICU4C_INCLUDE_DIR
  )
