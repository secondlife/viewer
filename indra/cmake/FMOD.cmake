# -*- cmake -*-

set(FMOD ON CACHE BOOL "Use FMOD sound library.")

if (FMOD)
  if (STANDALONE)
    set(FMOD_FIND_REQUIRED ON)
    include(FindFMOD)
  else (STANDALONE)
    include(Prebuilt)
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
endif (FMOD)
