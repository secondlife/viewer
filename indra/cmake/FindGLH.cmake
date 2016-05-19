# -*- cmake -*-

# - Find GLH
# Find the Graphic Library Helper includes.
# This module defines
#  GLH_INCLUDE_DIR, where to find glh/glh_linear.h.
#  GLH_FOUND, If false, do not try to use GLH.

find_path(GLH_INCLUDE_DIR glh/glh_linear.h
    NO_SYSTEM_ENVIRONMENT_PATH
    )

if (GLH_INCLUDE_DIR)
  set(GLH_FOUND "YES")
else (GLH_INCLUDE_DIR)
  set(GLH_FOUND "NO")
endif (GLH_INCLUDE_DIR)

if (GLH_FOUND)
  if (NOT GLH_FIND_QUIETLY)
    message(STATUS "Found GLH: ${GLH_INCLUDE_DIR}")
    set(GLH_FIND_QUIETLY TRUE) # Only alert us the first time
  endif (NOT GLH_FIND_QUIETLY)
else (GLH_FOUND)
  if (GLH_FIND_REQUIRED)
    message(FATAL_ERROR "Could not find GLH")
  endif (GLH_FIND_REQUIRED)
endif (GLH_FOUND)

mark_as_advanced(GLH_INCLUDE_DIR)
