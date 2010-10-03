# -*- cmake -*-
include(Prebuilt)

set(FMOD_FIND_QUIETLY OFF)
set(FMOD_FIND_REQUIRED OFF)

if (STANDALONE)
  include(FindFMOD)
else (STANDALONE)
  use_prebuilt_binary(fmod)
  
  if (WINDOWS)
    set(FMOD_LIBRARY fmod)
  elseif (DARWIN)
    set(FMOD_LIBRARY fmod)
  elseif (LINUX)
    set(FMOD_LIBRARY fmod-3.75)
  endif (WINDOWS)
  SET(FMOD_LIBRARIES ${FMOD_LIBRARY})
  
  set(FMOD_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/include)
endif (STANDALONE)

if (FMOD_LIBRARY AND FMOD_INCLUDE_DIR)
  set(FMOD ON CACHE BOOL "Use FMOD sound library.")
else (FMOD_LIBRARY AND FMOD_INCLUDE_DIR)
  set(FMOD_LIBRARY "")
  set(FMOD_INCLUDE_DIR "")
  if (FMOD)
    message(STATUS "No support for FMOD audio found.")
  endif (FMOD)
  set(FMOD OFF CACHE BOOL "FMOD sound library not used.")
endif (FMOD_LIBRARY AND FMOD_INCLUDE_DIR)

if (FMOD)
  message(STATUS "Building with FMOD audio support")
endif (FMOD)
