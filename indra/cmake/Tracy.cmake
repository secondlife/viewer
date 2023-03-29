# -*- cmake -*-
include(Prebuilt)

include_guard()
add_library( ll::tracy INTERFACE IMPORTED )

set(USE_TRACY OFF CACHE BOOL "Use Tracy profiler.")

if (USE_TRACY)
  use_system_binary(tracy)
  use_prebuilt_binary(tracy)

  target_include_directories( ll::tracy SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include/tracy)
    set(TRACY_LIBRARY "TracyClient")
    set(TRACY_LIBRARY "TracyClient")

# See: indra/llcommon/llprofiler.h
  target_compile_definitions(ll::tracy INTERFACE LL_PROFILER_CONFIGURATION=3 )
    set(TRACY_LIBRARY "TracyClient")
endif (USE_TRACY)

