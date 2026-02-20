# -*- cmake -*-
#
# Compared to other libraries, compiling this one is a mess.  The
# reason is that we have several source files that have two different
# sets of behaviour, depending on whether they're intended to be part
# of the viewer or the map server.
#
# Unfortunately, the affected code is a rat's nest of #ifdefs, so it's
# easier to play compilation tricks than to actually fix the problem.

include(UI)

set(llwindow_BASE_SOURCE_FILES
  llcursortypes.cpp
  llkeyboard.cpp
  llkeyboardheadless.cpp
  llmousehandler.cpp
  llwindow.cpp
  llwindowheadless.cpp
  llwindowcallbacks.cpp
)

set(llwindow_BASE_HEADER_FILES
  llcursortypes.h
  llkeyboard.h
  llkeyboardheadless.h
  llmousehandler.h
  llwindow.h
  llwindowheadless.h
  llwindowcallbacks.h
)

set(llwindow_BASE_LINK_LIBRARIES
  llcommon
  llimage
  llmath
  llfilesystem
  llxml
  ll::glm
)

add_library(llwindow STATIC)

target_sources(llwindow
  PRIVATE
    ${llwindow_BASE_SOURCE_FILES}
  PUBLIC
    ${llwindow_BASE_HEADER_FILES}
    llpreeditor.h
)

target_link_libraries(llwindow llrender ${llwindow_BASE_LINK_LIBRARIES} ll::uilibraries)
target_include_directories(llwindow INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_precompile_headers(llwindow REUSE_FROM llprecompiled)

if(USE_SDL_WINDOW)
  target_sources(llwindow
    PRIVATE
      llsdl.cpp
      llkeyboardsdl.cpp
      llwindowsdl.cpp
    PUBLIC
      llsdl.h
      llkeyboardsdl.h
      llwindowsdl.h
  )

  target_compile_definitions(llwindow PUBLIC LL_SDL_WINDOW=1)

  target_link_libraries(llwindow
    ll::SDL3
  )
endif()

if(DARWIN)
  target_sources(llwindow
    PRIVATE
      llkeyboardmacosx.cpp
      llwindowmacosx.cpp
      llwindowmacosx-objc.mm
      llopenglview-objc.mm
    PUBLIC
      llkeyboardmacosx.h
      llwindowmacosx.h
      llwindowmacosx-objc.h
      llopenglview-objc.h
      llappdelegate-objc.h
  )

  set_source_files_properties(
    llwindowmacosx-objc.mm
    llopenglview-objc.mm
    PROPERTIES
    SKIP_PRECOMPILE_HEADERS TRUE
  )
endif(DARWIN)

if(WINDOWS)
  target_sources(llwindow
    PRIVATE
      llwindowwin32.cpp
      lldxhardware.cpp
      llkeyboardwin32.cpp
      lldragdropwin32.cpp
    PUBLIC
      llwindowwin32.h
      lldxhardware.h
      llkeyboardwin32.h
      lldragdropwin32.h
  )
endif(WINDOWS)

if(BUILD_HEADLESS)
  add_library(llwindowheadless STATIC)

  target_sources(llwindowheadless
    PRIVATE
      ${llwindow_BASE_SOURCE_FILES} 
      llwindowmesaheadless.cpp
    PUBLIC
      ${llwindow_BASE_HEADER_FILES}
      llwindowmesaheadless.h
  )

  target_compile_definitions(llwindowheadless PUBLIC LL_MESA_HEADLESS=1)

  target_link_libraries(llwindowheadless
    llrenderheadless
    ${llwindow_BASE_LINK_LIBRARIES}
    PkgConfig::OSMESA
    dl
  )
  target_include_directories(llwindowheadless INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
  target_precompile_headers(llwindowheadless REUSE_FROM llprecompiled)
endif(BUILD_HEADLESS)
