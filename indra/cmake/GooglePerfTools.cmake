# -*- cmake -*-
include(Prebuilt)

if (STANDALONE)
  include(FindGooglePerfTools)
else (STANDALONE)
  if (WINDOWS)
    use_prebuilt_binary(tcmalloc)
    set(TCMALLOC_LIBRARIES 
        debug libtcmalloc_minimal-debug
        optimized libtcmalloc_minimal)
    set(GOOGLE_PERFTOOLS_FOUND "YES")
  endif (WINDOWS)
  if (LINUX)
    use_prebuilt_binary(tcmalloc)
    set(TCMALLOC_LIBRARIES 
    tcmalloc)
    set(PROFILER_LIBRARIES profiler)
    set(GOOGLE_PERFTOOLS_INCLUDE_DIR
        ${LIBS_PREBUILT_DIR}/include)
    set(GOOGLE_PERFTOOLS_FOUND "YES")
  endif (LINUX)
endif (STANDALONE)

if (GOOGLE_PERFTOOLS_FOUND)
  # XXX Disable temporarily, until we have compilation issues on 64-bit
  # Etch sorted.
  set(USE_GOOGLE_PERFTOOLS OFF CACHE BOOL "Build with Google PerfTools support.")
endif (GOOGLE_PERFTOOLS_FOUND)

if (WINDOWS)
    set(USE_GOOGLE_PERFTOOLS ON)
endif (WINDOWS)

if (USE_GOOGLE_PERFTOOLS)
  set(TCMALLOC_FLAG -ULL_USE_TCMALLOC=1)
  include_directories(${GOOGLE_PERFTOOLS_INCLUDE_DIR})
  set(GOOGLE_PERFTOOLS_LIBRARIES ${TCMALLOC_LIBRARIES} ${STACKTRACE_LIBRARIES} ${PROFILER_LIBRARIES})
else (USE_GOOGLE_PERFTOOLS)
  set(TCMALLOC_FLAG -ULL_USE_TCMALLOC)
endif (USE_GOOGLE_PERFTOOLS)
