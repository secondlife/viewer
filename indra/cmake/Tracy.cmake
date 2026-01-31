# -*- cmake -*-
include_guard()
add_library(ll::tracy INTERFACE IMPORTED)

if (USE_TRACY)
  if (USE_VCPKG)
    find_package(Tracy CONFIG REQUIRED)
    target_link_libraries(ll::tracy INTERFACE Tracy::TracyClient)
  else()
    include(Prebuilt)
    use_system_binary(tracy)
    use_prebuilt_binary(tracy)
    target_include_directories(ll::tracy SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include/tracy)
    target_compile_definitions(ll::tracy INTERFACE LL_BUILD_TRACY=1)
  endif()

  if (USE_TRACY_GPU AND NOT DARWIN) # Tracy OpenGL mode is incompatible with macOS/iOS
    target_compile_definitions(ll::tracy INTERFACE LL_PROFILER_ENABLE_TRACY_OPENGL=1)
  endif ()

  # See: indra/llcommon/llprofiler.h
  target_compile_definitions(ll::tracy INTERFACE LL_PROFILER_CONFIGURATION=3)
endif (USE_TRACY)

