# -*- cmake -*-

if (NOT STANDALONE)
  if (WINDOWS)
    set(ARCH_PREBUILT_DIRS ${LIBS_PREBUILT_DIR}/${LL_ARCH_DIR}/lib)
    set(ARCH_PREBUILT_DIRS_RELEASE ${LIBS_PREBUILT_DIR}/${LL_ARCH_DIR}/lib/release)
    set(ARCH_PREBUILT_DIRS_DEBUG ${LIBS_PREBUILT_DIR}/${LL_ARCH_DIR}/lib/debug)
  elseif (LINUX)
    if (VIEWER)
      set(ARCH_PREBUILT_DIRS 
          ${LIBS_PREBUILT_DIR}/${LL_ARCH_DIR}/lib_release_client)
    else (VIEWER)
      set(ARCH_PREBUILT_DIRS ${LIBS_PREBUILT_DIR}/${LL_ARCH_DIR}/lib_release)
    endif (VIEWER)
  elseif (DARWIN)
    set(ARCH_PREBUILT_DIRS_RELEASE ${LIBS_PREBUILT_DIR}/${LL_ARCH_DIR}/lib_release)
    set(ARCH_PREBUILT_DIRS_DEBUG ${ARCH_PREBUILT_DIRS_RELEASE})
  endif (WINDOWS)
endif (NOT STANDALONE)

link_directories(${ARCH_PREBUILT_DIRS})

if (LINUX)
  set(DL_LIBRARY dl)
  set(PTHREAD_LIBRARY pthread)
else (LINUX)
  set(DL_LIBRARY "")
  set(PTHREAD_LIBRARY "")
endif (LINUX)

if (WINDOWS)
  set(WINDOWS_LIBRARIES
      ws2_32
      mswsock
      psapi
      winmm
      netapi32
      )
else (WINDOWS)
  set(WINDOWS_LIBRARIES "")
endif (WINDOWS)
    
mark_as_advanced(DL_LIBRARY PTHREAD_LIBRARY WINDOWS_LIBRARIES)
