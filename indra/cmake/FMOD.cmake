# -*- cmake -*-

# FMOD can be set when launching the make using the argument -DFMOD:BOOL=ON
# When building using proprietary binaries though (i.e. having access to LL private servers),
# we always build with FMOD.
# Open source devs should use the -DFMOD:BOOL=ON then if they want to build with FMOD, whether
# they are using STANDALONE or not.
if (INSTALL_PROPRIETARY)
  set(FMOD ON CACHE BOOL "Use FMOD sound library.")
endif (INSTALL_PROPRIETARY)

if (FMOD)
  if (NOT INSTALL_PROPRIETARY)
    # This cover the STANDALONE case and the NOT STANDALONE but not using proprietary libraries
	# This should then be invoke by all open source devs outside LL
    set(FMOD_FIND_REQUIRED ON)
    include(FindFMOD)
  else (NOT INSTALL_PROPRIETARY)
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
  endif (NOT INSTALL_PROPRIETARY)
endif (FMOD)
