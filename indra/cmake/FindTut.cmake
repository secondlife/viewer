# -*- cmake -*-

# - Find Tut
# Find the Tut unit test framework includes and library
# This module defines
#  TUT_INCLUDE_DIR, where to find tut/tut.hpp.
#  TUT_FOUND, If false, do not try to use Tut.

find_path(TUT_INCLUDE_DIR tut/tut.hpp
    NO_SYSTEM_ENVIRONMENT_PATH
    )

if (TUT_INCLUDE_DIR)
  set(TUT_FOUND "YES")
else (TUT_INCLUDE_DIR)
  set(TUT_FOUND "NO")
endif (TUT_INCLUDE_DIR)

if (TUT_FOUND)
  if (NOT TUT_FIND_QUIETLY)
    message(STATUS "Found Tut: ${TUT_INCLUDE_DIR}")
    set(TUT_FIND_QUIETLY TRUE) # Only alert us the first time
  endif (NOT TUT_FIND_QUIETLY)
else (TUT_FOUND)
  if (TUT_FIND_REQUIRED)
    message(FATAL_ERROR "Could not find Tut")
  endif (TUT_FIND_REQUIRED)
endif (TUT_FOUND)

mark_as_advanced(TUT_INCLUDE_DIR)
