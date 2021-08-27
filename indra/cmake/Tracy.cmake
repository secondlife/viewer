# -*- cmake -*-
include(Prebuilt)

set(TRACY OFF CACHE BOOL "Use Tracy profiler.")

if (TRACY)
  set(TRACY_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/include/tracy) 

# See: indra/llcommon/llprofiler.h
  add_definitions(-DLL_PROFILER_CONFIGURATION=2)
  use_prebuilt_binary(tracy)

  if (WINDOWS)
    MESSAGE(STATUS "Including Tracy for Windows: '${TRACY_INCLUDE_DIR}'")
    set(TRACY_LIBRARY tracy)
  endif (WINDOWS)

  if (DARWIN)
    MESSAGE(STATUS "Including Tracy for Darwin: '${TRACY_INCLUDE_DIR}'")
    set(TRACY_LIBRARY "")
  endif (DARWIN)

  if (LINUX)
    MESSAGE(STATUS "Including Tracy for Linux: '${TRACY_INCLUDE_DIR}'")
    set(TRACY_LIBRARY "")
  endif (LINUX)
else (TRACY)
  set(TRACY_LIBRARY "")
endif (TRACY)

