# -*- cmake -*-

# - Find HUNSPELL
# This module defines
#  HUNSPELL_INCLUDE_DIR, where to find libhunspell.h, etc.
#  HUNSPELL_LIBRARY, the library needed to use HUNSPELL.
#  HUNSPELL_FOUND, If false, do not try to use HUNSPELL.

find_path(HUNSPELL_INCLUDE_DIR hunspell.h
  PATH_SUFFIXES hunspell
  )

set(HUNSPELL_NAMES ${HUNSPELL_NAMES} libhunspell-1.3.0 libhunspell)
find_library(HUNSPELL_LIBRARY
  NAMES ${HUNSPELL_NAMES}
  )

if (HUNSPELL_LIBRARY AND HUNSPELL_INCLUDE_DIR)
  set(HUNSPELL_FOUND "YES")
else (HUNSPELL_LIBRARY AND HUNSPELL_INCLUDE_DIR)
  set(HUNSPELL_FOUND "NO")
endif (HUNSPELL_LIBRARY AND HUNSPELL_INCLUDE_DIR)


if (HUNSPELL_FOUND)
  if (NOT HUNSPELL_FIND_QUIETLY)
    message(STATUS "Found Hunspell: Library in '${HUNSPELL_LIBRARY}' and header in '${HUNSPELL_INCLUDE_DIR}' ")
  endif (NOT HUNSPELL_FIND_QUIETLY)
else (HUNSPELL_FOUND)
  if (HUNSPELL_FIND_REQUIRED)
    message(FATAL_ERROR " * * *\nCould not find HUNSPELL library! * * *")
  endif (HUNSPELL_FIND_REQUIRED)
endif (HUNSPELL_FOUND)

mark_as_advanced(
  HUNSPELL_LIBRARY
  HUNSPELL_INCLUDE_DIR
  )
