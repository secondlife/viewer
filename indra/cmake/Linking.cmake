# -*- cmake -*-

if (NOT STANDALONE)
  if (WINDOWS)
    set(ARCH_PREBUILT_DIRS ${LIBS_PREBUILT_DIR}/${LL_ARCH_DIR}/lib)
    set(ARCH_PREBUILT_DIRS_RELEASE ${LIBS_PREBUILT_DIR}/${LL_ARCH_DIR}/lib/release)
    set(ARCH_PREBUILT_DIRS_DEBUG ${LIBS_PREBUILT_DIR}/${LL_ARCH_DIR}/lib/debug)
    set(SHARED_LIB_STAGING_DIR ${CMAKE_BINARY_DIR}/sharedlibs CACHE FILEPATH "Location of staged DLLs")
    set(EXE_STAGING_DIR ${CMAKE_BINARY_DIR}/sharedlibs CACHE FILEPATH "Location of staged executables")
  elseif (LINUX)
    if (VIEWER)
      set(ARCH_PREBUILT_DIRS ${LIBS_PREBUILT_DIR}/${LL_ARCH_DIR}/lib_release_client)
    else (VIEWER)
      set(ARCH_PREBUILT_DIRS ${LIBS_PREBUILT_DIR}/${LL_ARCH_DIR}/lib_release)
    endif (VIEWER)
    set(ARCH_PREBUILT_DIRS_RELEASE ${ARCH_PREBUILT_DIRS})
    set(ARCH_PREBUILT_DIRS_DEBUG ${ARCH_PREBUILT_DIRS})
    set(SHARED_LIB_STAGING_DIR ${CMAKE_BINARY_DIR}/sharedlibs/lib CACHE FILEPATH "Location of staged .sos")
    set(EXE_STAGING_DIR ${CMAKE_BINARY_DIR}/sharedlibs/bin CACHE FILEPATH "Location of staged executables")
  elseif (DARWIN)
    set(ARCH_PREBUILT_DIRS_RELEASE ${LIBS_PREBUILT_DIR}/${LL_ARCH_DIR}/lib_release)
    set(ARCH_PREBUILT_DIRS ${ARCH_PREBUILT_DIRS_RELEASE})
    set(ARCH_PREBUILT_DIRS_DEBUG ${ARCH_PREBUILT_DIRS_RELEASE})
    set(SHARED_LIB_STAGING_DIR ${CMAKE_BINARY_DIR}/sharedlibs CACHE FILEPATH "Location of staged DLLs")
    set(EXE_STAGING_DIR "${CMAKE_BINARY_DIR}/sharedlibs/\$(CONFIGURATION)" CACHE FILEPATH "Location of staged executables")
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
      advapi32
      shell32
      ws2_32
      mswsock
      psapi
      winmm
      netapi32
      wldap32
      gdi32
      user32
      dbghelp
      )
else (WINDOWS)
  set(WINDOWS_LIBRARIES "")
endif (WINDOWS)
    
mark_as_advanced(DL_LIBRARY PTHREAD_LIBRARY WINDOWS_LIBRARIES)
