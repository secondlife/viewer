# -*- cmake -*-

include_guard()
include(Variables)

set(ARCH_PREBUILT_DIRS ${AUTOBUILD_INSTALL_DIR}/lib)
set(ARCH_PREBUILT_DIRS_PLUGINS ${AUTOBUILD_INSTALL_DIR}/plugins)
set(ARCH_PREBUILT_DIRS_RELEASE ${AUTOBUILD_INSTALL_DIR}/lib/release)
set(ARCH_PREBUILT_DIRS_DEBUG ${AUTOBUILD_INSTALL_DIR}/lib/debug)
if (WINDOWS)
  set(SHARED_LIB_STAGING_DIR ${CMAKE_BINARY_DIR}/sharedlibs)
  set(EXE_STAGING_DIR ${CMAKE_BINARY_DIR}/sharedlibs)
elseif (LINUX)
  set(SHARED_LIB_STAGING_DIR ${CMAKE_BINARY_DIR}/sharedlibs/lib)
  set(EXE_STAGING_DIR ${CMAKE_BINARY_DIR}/sharedlibs/bin)
elseif (DARWIN)
  set(SHARED_LIB_STAGING_DIR ${CMAKE_BINARY_DIR}/sharedlibs)
  set(EXE_STAGING_DIR "${CMAKE_BINARY_DIR}/sharedlibs")
  if( ${CMAKE_GENERATOR} STREQUAL "Ninja")
    set(EXE_STAGING_DIR "${CMAKE_BINARY_DIR}/sharedlibs/${CMAKE_BUILD_TYPE} ")
  else()
    set(EXE_STAGING_DIR "${CMAKE_BINARY_DIR}/sharedlibs")
  endif()
endif (WINDOWS)

# Autobuild packages must provide 'release' versions of libraries, but may provide versions for
# specific build types.  AUTOBUILD_LIBS_INSTALL_DIRS lists first the build type directory and then
# the 'release' directory (as a default fallback).
# *NOTE - we have to take special care to use CMAKE_CFG_INTDIR on IDE generators (like mac and
# windows) and CMAKE_BUILD_TYPE on Makefile based generators (like linux).  The reason for this is
# that CMAKE_BUILD_TYPE is essentially meaningless at configuration time for IDE generators and
# CMAKE_CFG_INTDIR is meaningless at build time for Makefile generators

link_directories(${AUTOBUILD_INSTALL_DIR}/lib/$<LOWER_CASE:$<CONFIG>>)
link_directories(${AUTOBUILD_INSTALL_DIR}/lib/release)

add_library( ll::oslibraries INTERFACE IMPORTED )

if (LINUX)
  target_link_libraries( ll::oslibraries INTERFACE
          dl
          pthread
          rt)
elseif (WINDOWS)
  target_link_libraries( ll::oslibraries INTERFACE
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
          ole32
          dbghelp
          legacy_stdio_definitions
          )
else()
  include(CMakeFindFrameworks)
  find_library(COREFOUNDATION_LIBRARY CoreFoundation)
  find_library(CARBON_LIBRARY Carbon)
  find_library(COCOA_LIBRARY Cocoa)
  find_library(IOKIT_LIBRARY IOKit)

  find_library(AGL_LIBRARY AGL)
  find_library(APPKIT_LIBRARY AppKit)
  find_library(COREAUDIO_LIBRARY CoreAudio)

  target_link_libraries( ll::oslibraries INTERFACE
          ${COCOA_LIBRARY}
          ${IOKIT_LIBRARY}
          ${COREFOUNDATION_LIBRARY}
          ${CARBON_LIBRARY}
          ${AGL_LIBRARY}
          ${APPKIT_LIBRARY}
          ${COREAUDIO_LIBRARY}
          )
endif()



