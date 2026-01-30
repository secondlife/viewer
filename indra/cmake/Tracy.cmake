# -*- cmake -*-
include_guard()
add_library(ll::tracy INTERFACE IMPORTED)

if (USE_TRACY)
  find_package(Tracy CONFIG REQUIRED)
  target_link_libraries(ll::tracy INTERFACE Tracy::TracyClient)

  if (USE_TRACY_GPU AND NOT DARWIN) # Tracy OpenGL mode is incompatible with macOS/iOS
    target_compile_definitions(ll::tracy INTERFACE LL_PROFILER_ENABLE_TRACY_OPENGL=1)
  endif ()

  # See: indra/llcommon/llprofiler.h
  target_compile_definitions(ll::tracy INTERFACE LL_PROFILER_CONFIGURATION=3)
endif (USE_TRACY)

