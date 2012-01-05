# -*- cmake -*-
include(Prebuilt)

# If you want to enable or disable TCMALLOC in viewer builds, this is the place.
# set ON or OFF as desired.
set (USE_TCMALLOC ON)

if (STANDALONE)
  include(FindGooglePerfTools)
else (STANDALONE)
  if (WINDOWS)
    if (USE_TCMALLOC)
       use_prebuilt_binary(tcmalloc)
       set(TCMALLOC_LIBRARIES 
         debug libtcmalloc_minimal-debug
         optimized libtcmalloc_minimal)
    else (USE_TCMALLOC)
      set(TCMALLOC_LIBRARIES)
    endif (USE_TCMALLOC)
    set(GOOGLE_PERFTOOLS_FOUND "YES")
  endif (WINDOWS)
  if (LINUX)
    if (USE_TCMALLOC)
      use_prebuilt_binary(tcmalloc)
      set(TCMALLOC_LIBRARIES 
        tcmalloc)
    else (USE_TCMALLOC)
      set(TCMALLOC_LIBRARIES)
    endif (USE_TCMALLOC)
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

# Apparently buggy - LL_USE_TCMALLOC never gets set. Fix when we have time to test the
# corresponding source code.
if (USE_TCMALLOC)
  set(TCMALLOC_FLAG -ULL_USE_TCMALLOC=1)
else (USE_TCMALLOC)
  set(TCMALLOC_FLAG -ULL_USE_TCMALLOC)
endif (USE_TCMALLOC)

if (USE_GOOGLE_PERFTOOLS)
  include_directories(${GOOGLE_PERFTOOLS_INCLUDE_DIR})
  set(GOOGLE_PERFTOOLS_LIBRARIES ${TCMALLOC_LIBRARIES} ${STACKTRACE_LIBRARIES} ${PROFILER_LIBRARIES})
else (USE_GOOGLE_PERFTOOLS)
endif (USE_GOOGLE_PERFTOOLS)
