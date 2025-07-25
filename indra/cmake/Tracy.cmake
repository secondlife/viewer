# -*- cmake -*-
include(Prebuilt)

include_guard()
add_library( ll::tracy INTERFACE IMPORTED )

# default Tracy profiling on for test builds, but off for all others
string(TOLOWER ${VIEWER_CHANNEL} channel_lower)
if(channel_lower MATCHES "^second life test")
  option(USE_TRACY "Use Tracy profiler." ON)
else()
    option(USE_TRACY "Use Tracy profiler." OFF)
endif()

if (USE_TRACY)
  option(USE_TRACY_ON_DEMAND "Use on-demand Tracy profiling." ON)
  option(USE_TRACY_LOCAL_ONLY "Disallow remote Tracy profiling." OFF)

  use_system_binary(tracy)
  use_prebuilt_binary(tracy)

  target_include_directories( ll::tracy SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include/tracy)

  target_compile_definitions(ll::tracy INTERFACE -DTRACY_ENABLE=1 -DTRACY_ONLY_IPV4=1)

  if (USE_TRACY_ON_DEMAND)
    target_compile_definitions(ll::tracy INTERFACE -DTRACY_ON_DEMAND=1)
  endif ()

  if (USE_TRACY_LOCAL_ONLY)
    target_compile_definitions(ll::tracy INTERFACE -DTRACY_NO_BROADCAST=1 -DTRACY_ONLY_LOCALHOST=1)
  endif ()

  # GHA runners don't always provide invariant TSC support, but always build with LL_TESTS enabled
  if (DARWIN AND LL_TESTS)
    target_compile_definitions(ll::tracy INTERFACE -DTRACY_TIMER_FALLBACK=1)
  endif ()

  # See: indra/llcommon/llprofiler.h
  add_compile_definitions(LL_PROFILER_CONFIGURATION=3)
endif (USE_TRACY)

