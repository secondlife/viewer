# -*- cmake -*-

include_guard()
include(Variables)

set(SYMBOLS_STAGING_DIR ${INDRA_BINARY_DIR}/symbols/$<IF:$<BOOL:${LL_GENERATOR_IS_MULTI_CONFIG}>,$<CONFIG>/,>${VIEWER_CHANNEL})

if (WINDOWS OR DARWIN)
  set(SHARED_LIB_STAGING_DIR ${INDRA_BINARY_DIR}/sharedlibs/$<IF:$<BOOL:${LL_GENERATOR_IS_MULTI_CONFIG}>,$<CONFIG>,>)

  if(DARWIN)
    set(SHARED_LIB_STAGING_DIR ${SHARED_LIB_STAGING_DIR}/Frameworks)
    set(VIEWER_STAGING_DIR ${INDRA_BINARY_DIR}/newview/$<IF:$<BOOL:${LL_GENERATOR_IS_MULTI_CONFIG}>,$<CONFIG>,>)
  else()
    set(VIEWER_STAGING_DIR ${INDRA_BINARY_DIR}/newview/$<IF:$<BOOL:${LL_GENERATOR_IS_MULTI_CONFIG}>,$<CONFIG>,packaged>)
  endif()
  set(EXE_STAGING_DIR ${INDRA_BINARY_DIR}/sharedlibs/$<IF:$<BOOL:${LL_GENERATOR_IS_MULTI_CONFIG}>,$<CONFIG>,>)
elseif(LINUX)
  set(SHARED_LIB_STAGING_DIR ${INDRA_BINARY_DIR}/sharedlibs/$<IF:$<BOOL:${LL_GENERATOR_IS_MULTI_CONFIG}>,$<CONFIG>,>/lib)
  set(EXE_STAGING_DIR ${INDRA_BINARY_DIR}/sharedlibs/$<IF:$<BOOL:${LL_GENERATOR_IS_MULTI_CONFIG}>,$<CONFIG>,>/bin)
  set(VIEWER_STAGING_DIR ${INDRA_BINARY_DIR}/newview/$<IF:$<BOOL:${LL_GENERATOR_IS_MULTI_CONFIG}>,$<CONFIG>,packaged>)
endif()

add_library(ll::oslibraries INTERFACE IMPORTED)

if(LINUX)
  target_link_libraries(ll::oslibraries INTERFACE
          ${CMAKE_DL_LIBS}
          Threads::Threads
          rt)
elseif (WINDOWS)
  target_link_libraries(ll::oslibraries INTERFACE
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



