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
  if (STANDALONE)
    # In that case, we use the version of the library installed on the system
    set(FMOD_FIND_REQUIRED ON)
    include(FindFMOD)
  else (STANDALONE)
    if (FMOD_LIBRARY AND FMOD_INCLUDE_DIR)
	  # If the path have been specified in the arguments, use that
      set(FMOD_LIBRARIES ${FMOD_LIBRARY})
	  MESSAGE(STATUS "Using FMOD path: ${FMOD_LIBRARIES}, ${FMOD_INCLUDE_DIR}")
    else (FMOD_LIBRARY AND FMOD_INCLUDE_DIR)
	  # If not, we're going to try to get the package listed in autobuild.xml
	  # Note: if you're not using INSTALL_PROPRIETARY, the package URL should be local (file:/// URL) 
	  # as accessing the private LL location will fail if you don't have the credential
	  include(Prebuilt)
	  use_prebuilt_binary(fmod)    
      if (WINDOWS)
        set(FMOD_LIBRARY fmod)
      elseif (DARWIN)
        set(FMOD_LIBRARY fmod)
      elseif (LINUX)
        set(FMOD_LIBRARY fmod-3.75)
      endif (WINDOWS)
      set(FMOD_LIBRARIES ${FMOD_LIBRARY})
      set(FMOD_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/include)
    endif (FMOD_LIBRARY AND FMOD_INCLUDE_DIR)
  endif (STANDALONE)
endif (FMOD)
