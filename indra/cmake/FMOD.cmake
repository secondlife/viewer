# -*- cmake -*-

find_library(FMOD_LIBRARY
             NAMES fmod fmodvc fmod-3.75
             PATHS
             optimized ${ARCH_PREBUILT_DIRS_RELEASE}
             debug ${ARCH_PREBUILT_DIRS_DEBUG}
             ${LIBS_PREBUILT_DIR}/${LL_ARCH_DIR}/lib/release
             ${LIBS_PREBUILT_DIR}/${LL_ARCH_DIR}/lib_release
             ${LIBS_PREBUILT_DIR}/${LL_ARCH_DIR}/lib_release_client
             )

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
