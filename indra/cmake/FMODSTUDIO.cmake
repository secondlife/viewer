# -*- cmake -*-

# FMODSTUDIO can be set when launching the make using the argument -DFMODSTUDIO:BOOL=ON
# When building using proprietary binaries though (i.e. having access to LL private servers),
# we always build with FMODSTUDIO.
if (INSTALL_PROPRIETARY)
  set(FMODSTUDIO ON CACHE BOOL "Using FMODSTUDIO sound library.")
endif (INSTALL_PROPRIETARY)

if (FMODSTUDIO)
  if( TARGET fmodstudio::fmodstudio )
    return()
  endif()
  create_target( fmodstudio::fmodstudio )
  set_target_include_dirs( openal::openal "${LIBS_PREBUILT_DIR}/include/AL")

  if (FMODSTUDIO_LIBRARY AND FMODSTUDIO_INCLUDE_DIR)
    # If the path have been specified in the arguments, use that

    set_target_libraries(fmodstudio::fmodstudio ${FMODSTUDIO_LIBRARY})
    set_target_include_dirs(fmodstudio::fmodstudio ${FMODSTUDIO_INCLUDE_DIR})
  else (FMODSTUDIO_LIBRARY AND FMODSTUDIO_INCLUDE_DIR)
    # If not, we're going to try to get the package listed in autobuild.xml
    # Note: if you're not using INSTALL_PROPRIETARY, the package URL should be local (file:/// URL)
    # as accessing the private LL location will fail if you don't have the credential
    include(Prebuilt)
    use_prebuilt_binary(fmodstudio)
    if (WINDOWS)
      set_target_libraries( fmodstudio::fmodstudio  fmod_vc)
    elseif (DARWIN)
      #despite files being called libfmod.dylib, we are searching for fmod
      set_target_libraries( fmodstudio::fmodstudio  fmod)
    elseif (LINUX)
      set_target_libraries( fmodstudio::fmodstudio  fmod)
    endif (WINDOWS)

    set_target_include_dirs(fmodstudio::fmodstudio ${LIBS_PREBUILT_DIR}/include/fmodstudio)
  endif (FMODSTUDIO_LIBRARY AND FMODSTUDIO_INCLUDE_DIR)
endif (FMODSTUDIO)

