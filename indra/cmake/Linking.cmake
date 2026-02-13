# -*- cmake -*-

include_guard()
include(Variables)

if (WINDOWS OR DARWIN)
  # Kludge for older cmake versions, 3.20+ is needed to use a genex in add_custom_command( OUTPUT <var> ... )
  # Using this will work okay-ish, as Debug is not supported anyway. But for property multi config and also
  # ninja support the genex version is preferred.
  if(${CMAKE_VERSION} VERSION_LESS "3.20.0")
    if(CMAKE_BUILD_TYPE MATCHES Release)
      set(SHARED_LIB_STAGING_DIR ${CMAKE_BINARY_DIR}/sharedlibs/Release)
    elseif (CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
      set(SHARED_LIB_STAGING_DIR ${CMAKE_BINARY_DIR}/sharedlibs/RelWithDebInfo)
    endif()
  else()
    set(SHARED_LIB_STAGING_DIR ${CMAKE_BINARY_DIR}/sharedlibs/$<IF:$<BOOL:${LL_GENERATOR_IS_MULTI_CONFIG}>,$<CONFIG>,>)
    set(SYMBOLS_STAGING_DIR ${CMAKE_BINARY_DIR}/symbols/$<IF:$<BOOL:${LL_GENERATOR_IS_MULTI_CONFIG}>,$<CONFIG>,>/${VIEWER_CHANNEL})
  endif()

  if( DARWIN )
    set( SHARED_LIB_STAGING_DIR ${SHARED_LIB_STAGING_DIR}/Resources)
  endif()
  set(EXE_STAGING_DIR ${CMAKE_BINARY_DIR}/sharedlibs/$<IF:$<BOOL:${LL_GENERATOR_IS_MULTI_CONFIG}>,$<CONFIG>,>)
elseif (LINUX)
  set(SHARED_LIB_STAGING_DIR ${CMAKE_BINARY_DIR}/sharedlibs/lib)
  set(EXE_STAGING_DIR ${CMAKE_BINARY_DIR}/sharedlibs/bin)
endif ()

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
          rpcrt4.lib
          legacy_stdio_definitions
          )
else()
  find_library(COREFOUNDATION_LIBRARY CoreFoundation)
  find_library(CARBON_LIBRARY Carbon)
  find_library(COCOA_LIBRARY Cocoa)
  find_library(IOKIT_LIBRARY IOKit)

  find_library(APPKIT_LIBRARY AppKit)
  find_library(COREAUDIO_LIBRARY CoreAudio)
  find_library(COREGRAPHICS_LIBRARY CoreGraphics)
  find_library(AUDIOTOOLBOX_LIBRARY AudioToolbox)

  target_link_libraries( ll::oslibraries INTERFACE
          ${COCOA_LIBRARY}
          ${IOKIT_LIBRARY}
          ${COREFOUNDATION_LIBRARY}
          ${CARBON_LIBRARY}
          ${APPKIT_LIBRARY}
          ${COREAUDIO_LIBRARY}
          ${AUDIOTOOLBOX_LIBRARY}
          ${COREGRAPHICS_LIBRARY}
          )
endif()



