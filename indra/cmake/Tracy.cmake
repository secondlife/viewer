# -*- cmake -*-
include(Prebuilt)

#test build
set(USE_TRACY ON)

if (USE_TRACY)
  set(TRACY_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/include/tracy) 

# See: indra/llcommon/llprofiler.h
  add_definitions(-DLL_PROFILER_CONFIGURATION=2)
  use_prebuilt_binary(tracy)

  if (WINDOWS)
    MESSAGE(STATUS "Including Tracy for Windows: '${TRACY_INCLUDE_DIR}'")
  endif (WINDOWS)

  if (DARWIN)
    MESSAGE(STATUS "Including Tracy for Darwin: '${TRACY_INCLUDE_DIR}'")
  endif (DARWIN)

  if (LINUX)
    MESSAGE(STATUS "Including Tracy for Linux: '${TRACY_INCLUDE_DIR}'")
  endif (LINUX)
else (USE_TRACY)
  # Tracy.cmake should not set LLCOMMON_INCLUDE_DIRS, let LLCommon.cmake do that
  set(TRACY_INCLUDE_DIR "")
  set(TRACY_LIBRARY "")
endif (USE_TRACY)

