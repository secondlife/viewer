# -*- cmake -*-

include_guard()

# FMODSTUDIO can be set when launching the make using the argument -DFMODSTUDIO:BOOL=ON
# When building using proprietary binaries though (i.e. having access to LL private servers),
# we always build with FMODSTUDIO.
if (INSTALL_PROPRIETARY)
  set(USE_FMODSTUDIO ON CACHE BOOL "Using FMODSTUDIO sound library.")
endif (INSTALL_PROPRIETARY)

# ND: To streamline arguments passed, switch from FMODSTUDIO to USE_FMODSTUDIO
# To not break all old build scripts convert old arguments but warn about it
if(FMODSTUDIO)
  message( WARNING "Use of the FMODSTUDIO argument is deprecated, please switch to USE_FMODSTUDIO")
  set(USE_FMODSTUDIO ${FMODSTUDIO})
endif()

if (USE_FMODSTUDIO)
  add_library( ll::fmodstudio INTERFACE IMPORTED )
  target_compile_definitions( ll::fmodstudio INTERFACE LL_FMODSTUDIO=1)

  if (FMODSTUDIO_LIBRARY AND FMODSTUDIO_INCLUDE_DIR)
    # If the path have been specified in the arguments, use that

    target_link_libraries(ll::fmodstudio INTERFACE ${FMODSTUDIO_LIBRARY})
    target_include_directories( ll::fmodstudio SYSTEM INTERFACE  ${FMODSTUDIO_INCLUDE_DIR})
  else (FMODSTUDIO_LIBRARY AND FMODSTUDIO_INCLUDE_DIR)
    # If not, we're going to try to get the package listed in autobuild.xml
    # Note: if you're not using INSTALL_PROPRIETARY, the package URL should be local (file:/// URL)
    # as accessing the private LL location will fail if you don't have the credential
    include(Prebuilt)
    use_prebuilt_binary(fmodstudio)
    if (WINDOWS)
      target_link_libraries( ll::fmodstudio INTERFACE  fmod_vc)
    elseif (DARWIN)
      #despite files being called libfmod.dylib, we are searching for fmod
      target_link_libraries( ll::fmodstudio INTERFACE  fmod)
    elseif (LINUX)
      target_link_libraries( ll::fmodstudio INTERFACE  fmod)
    endif (WINDOWS)

    target_include_directories( ll::fmodstudio SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include/fmodstudio)
  endif (FMODSTUDIO_LIBRARY AND FMODSTUDIO_INCLUDE_DIR)
else()
  set( USE_FMODSTUDIO "OFF")
endif ()

