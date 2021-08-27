# -*- cmake -*-
include(Prebuilt)

set(TRACY OFF CACHE BOOL "Use Tracy profiler.")

if (TRACY)
  set(TRACY_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/include/tracy) 
  if (WINDOWS)
    use_prebuilt_binary(tracy)
    set(TRACY_LIBRARY tracy)
  else (WINDOWS)
    set(TRACY_LIBRARY "")
  endif (WINDOWS)
else (TRACY)
  set(TRACY_LIBRARY "")
endif (TRACY)

