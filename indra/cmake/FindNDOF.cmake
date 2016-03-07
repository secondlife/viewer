# -*- cmake -*-

# - Find NDOF
# Find the NDOF includes and library
# This module defines
#  NDOF_INCLUDE_DIR, where to find ndofdev_external.h, etc.
#  NDOF_LIBRARY, the library needed to use NDOF.
#  NDOF_FOUND, If false, do not try to use NDOF.

find_path(NDOF_INCLUDE_DIR ndofdev_external.h
  PATH_SUFFIXES ndofdev
  )

set(NDOF_NAMES ${NDOF_NAMES} ndofdev libndofdev)
find_library(NDOF_LIBRARY
  NAMES ${NDOF_NAMES}
  )

if (NDOF_LIBRARY AND NDOF_INCLUDE_DIR)
  set(NDOF_FOUND "YES")
else (NDOF_LIBRARY AND NDOF_INCLUDE_DIR)
  set(NDOF_FOUND "NO")
endif (NDOF_LIBRARY AND NDOF_INCLUDE_DIR)


if (NDOF_FOUND)
  if (NOT NDOF_FIND_QUIETLY)
    message(STATUS "Found NDOF: Library in '${NDOF_LIBRARY}' and header in '${NDOF_INCLUDE_DIR}' ")
  endif (NOT NDOF_FIND_QUIETLY)
else (NDOF_FOUND)
  if (NDOF_FIND_REQUIRED)
    message(FATAL_ERROR " * * *\nCould not find NDOF library!\nIf you don't need Space Navigator Joystick support you can skip this test by configuring with -DNDOF:BOOL=OFF\n * * *")
  endif (NDOF_FIND_REQUIRED)
endif (NDOF_FOUND)

mark_as_advanced(
  NDOF_LIBRARY
  NDOF_INCLUDE_DIR
  )
