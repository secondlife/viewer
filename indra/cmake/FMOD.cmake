# -*- cmake -*-

include(Linking)

if(INSTALL_PROPRIETARY)
  include(Prebuilt)
  use_prebuilt_binary(fmod)
endif(INSTALL_PROPRIETARY)

find_library(FMOD_LIBRARY_RELEASE
             NAMES fmod fmodvc fmod-3.75
             PATHS
             ${ARCH_PREBUILT_DIRS_RELEASE}
             )

find_library(FMOD_LIBRARY_DEBUG
             NAMES fmod fmodvc fmod-3.75
             PATHS
             ${ARCH_PREBUILT_DIRS_DEBUG}
             )

if (FMOD_LIBRARY_RELEASE AND FMOD_LIBRARY_DEBUG)
  set(FMOD_LIBRARY
      debug ${FMOD_LIBRARY_DEBUG}
      optimized ${FMOD_LIBRARY_RELEASE})
elseif (FMOD_LIBRARY_RELEASE)
  set(FMOD_LIBRARY ${FMOD_LIBRARY_RELEASE})
endif (FMOD_LIBRARY_RELEASE AND FMOD_LIBRARY_DEBUG)

if (NOT FMOD_LIBRARY)
  set(FMOD_SDK_DIR CACHE PATH "Path to the FMOD SDK.")
  if (FMOD_SDK_DIR)
    find_library(FMOD_LIBRARY
                 NAMES fmodvc fmod-3.75 fmod
                 PATHS
                 ${FMOD_SDK_DIR}/api/lib
                 ${FMOD_SDK_DIR}/api
                 ${FMOD_SDK_DIR}/lib
                 ${FMOD_SDK_DIR}
                 )
  endif (FMOD_SDK_DIR)
endif (NOT FMOD_LIBRARY)

find_path(FMOD_INCLUDE_DIR fmod.h
          ${LIBS_PREBUILT_DIR}/include
          ${FMOD_SDK_DIR}/api/inc
          ${FMOD_SDK_DIR}/inc
          ${FMOD_SDK_DIR}
          )

if (FMOD_LIBRARY AND FMOD_INCLUDE_DIR)
  set(FMOD ON CACHE BOOL "Use closed source FMOD sound library.")
else (FMOD_LIBRARY AND FMOD_INCLUDE_DIR)
  set(FMOD_LIBRARY "")
  set(FMOD_INCLUDE_DIR "")
  if (FMOD)
    message(STATUS "No support for FMOD audio (need to set FMOD_SDK_DIR?)")
  endif (FMOD)
  set(FMOD OFF CACHE BOOL "Use closed source FMOD sound library.")
endif (FMOD_LIBRARY AND FMOD_INCLUDE_DIR)

if (FMOD)
  message(STATUS "Building with FMOD audio support")
endif (FMOD)
