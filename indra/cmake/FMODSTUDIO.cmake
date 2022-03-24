# -*- cmake -*-

# FMODSTUDIO can be set when launching the make using the argument -DFMODSTUDIO:BOOL=ON
# When building using proprietary binaries though (i.e. having access to LL private servers),
# we always build with FMODSTUDIO.
if (INSTALL_PROPRIETARY)
  set(FMODSTUDIO ON CACHE BOOL "Using FMODSTUDIO sound library.")
endif (INSTALL_PROPRIETARY)

if (FMODSTUDIO)
  if (FMODSTUDIO_LIBRARY AND FMODSTUDIO_INCLUDE_DIR)
    # If the path have been specified in the arguments, use that
    set(FMODSTUDIO_LIBRARIES ${FMODSTUDIO_LIBRARY})
  else (FMODSTUDIO_LIBRARY AND FMODSTUDIO_INCLUDE_DIR)
    # If not, we're going to try to get the package listed in autobuild.xml
    # Note: if you're not using INSTALL_PROPRIETARY, the package URL should be local (file:/// URL)
    # as accessing the private LL location will fail if you don't have the credential
    include(Prebuilt)
    use_prebuilt_binary(fmodstudio)
    if (WINDOWS)
      set(FMODSTUDIO_LIBRARY
          debug fmodL_vc
          optimized fmod_vc)
    elseif (DARWIN)
      #despite files being called libfmod.dylib, we are searching for fmod
      set(FMODSTUDIO_LIBRARY
          debug fmodL
          optimized fmod)
    elseif (LINUX)
      set(FMODSTUDIO_LIBRARY
          debug fmodL
          optimized fmod)
    endif (WINDOWS)
    set(FMODSTUDIO_LIBRARIES ${FMODSTUDIO_LIBRARY})
    set(FMODSTUDIO_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/include/fmodstudio)
  endif (FMODSTUDIO_LIBRARY AND FMODSTUDIO_INCLUDE_DIR)
endif (FMODSTUDIO)

