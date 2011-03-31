# -*- cmake -*-

# - Find llqtwebkit
# Find the llqtwebkit includes and library
# This module defines
#  LLQTWEBKIT_INCLUDE_DIR, where to find llqtwebkit.h, etc.
#  LLQTWEBKIT_LIBRARY, the llqtwebkit library with full path.
#  LLQTWEBKIT_FOUND, If false, do not try to use llqtwebkit.
# also defined, but not for general use are
#  LLQTWEBKIT_LIBRARIES, the libraries needed to use llqtwebkit.
#  LLQTWEBKIT_LIBRARY_DIRS, where to find the llqtwebkit library.
#  LLQTWEBKIT_DEFINITIONS - You should add_definitions(${LLQTWEBKIT_DEFINITIONS})
#      before compiling code that includes llqtwebkit library files.

# Try to use pkg-config first.
# This allows to have two different libllqtwebkit packages installed:
# one for viewer 2.x and one for viewer 1.x.
include(FindPkgConfig)
if (PKG_CONFIG_FOUND)
    if (LLQtWebkit_FIND_REQUIRED AND LLQtWebkit_FIND_VERSION)
        set(_PACKAGE_ARGS libllqtwebkit>=${LLQtWebkit_FIND_VERSION} REQUIRED)
    else (LLQtWebkit_FIND_REQUIRED AND LLQtWebkit_FIND_VERSION)
        set(_PACKAGE_ARGS libllqtwebkit)
    endif (LLQtWebkit_FIND_REQUIRED AND LLQtWebkit_FIND_VERSION)
    if (NOT "${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}" VERSION_LESS "2.8.2")
      # As virtually nobody will have a pkg-config file for this, do this check always quiet.
      # Unfortunately cmake 2.8.2 or higher is required for pkg_check_modules to have a 'QUIET'.
      set(_PACKAGE_ARGS ${_PACKAGE_ARGS} QUIET)
    endif ()
    pkg_check_modules(LLQTWEBKIT ${_PACKAGE_ARGS})
endif (PKG_CONFIG_FOUND)
set(LLQTWEBKIT_DEFINITIONS ${LLQTWEBKIT_CFLAGS_OTHER})

find_path(LLQTWEBKIT_INCLUDE_DIR llqtwebkit.h NO_SYSTEM_ENVIRONMENT_PATH HINTS ${LLQTWEBKIT_INCLUDE_DIRS})

find_library(LLQTWEBKIT_LIBRARY NAMES llqtwebkit NO_SYSTEM_ENVIRONMENT_PATH HINTS ${LLQTWEBKIT_LIBRARY_DIRS})

if (NOT PKG_CONFIG_FOUND OR NOT LLQTWEBKIT_FOUND)	# If pkg-config couldn't find it, pretend we don't have pkg-config.
   set(LLQTWEBKIT_LIBRARIES llqtwebkit)
   get_filename_component(LLQTWEBKIT_LIBRARY_DIRS ${LLQTWEBKIT_LIBRARY} PATH)
endif (NOT PKG_CONFIG_FOUND OR NOT LLQTWEBKIT_FOUND)

# Handle the QUIETLY and REQUIRED arguments and set LLQTWEBKIT_FOUND
# to TRUE if all listed variables are TRUE.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  LLQTWEBKIT
  DEFAULT_MSG
  LLQTWEBKIT_LIBRARY
  LLQTWEBKIT_INCLUDE_DIR
  LLQTWEBKIT_LIBRARIES
  LLQTWEBKIT_LIBRARY_DIRS
  )

mark_as_advanced(
  LLQTWEBKIT_LIBRARY
  LLQTWEBKIT_INCLUDE_DIR
  LLQTWEBKIT_LIBRARIES
  LLQTWEBKIT_LIBRARY_DIRS
  LLQTWEBKIT_DEFINITIONS
  )

