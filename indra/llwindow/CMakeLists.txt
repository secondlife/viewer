# -*- cmake -*-
#
# Compared to other libraries, compiling this one is a mess.  The
# reason is that we have several source files that have two different
# sets of behaviour, depending on whether they're intended to be part
# of the viewer or the map server.
#
# Unfortunately, the affected code is a rat's nest of #ifdefs, so it's
# easier to play compilation tricks than to actually fix the problem.

project(llwindow)

include(00-Common)
include(DragDrop)
include(Linking)
include(LLCommon)
include(LLImage)
include(LLWindow)
include(UI)
include(ViewerMiscLibs)
include(GLM)
include(SDL3)

set(llwindow_SOURCE_FILES
    llcursortypes.cpp
    llkeyboard.cpp
    llkeyboardheadless.cpp
    llwindowheadless.cpp
    llwindowcallbacks.cpp
    llwindow.cpp
    )

set(llwindow_HEADER_FILES
    CMakeLists.txt

    llcursortypes.h
    llkeyboard.h
    llkeyboardheadless.h
    llwindowheadless.h
    llwindowcallbacks.h
    )

set(viewer_SOURCE_FILES
    llmousehandler.cpp
    )

set(viewer_HEADER_FILES
    llwindow.h
    llpreeditor.h
    llmousehandler.h
    )

set(llwindow_LINK_LIBRARIES
        llcommon
        llimage
        llmath
        llrender
        llfilesystem
        llxml
        ll::glm
        ll::glext
        ll::uilibraries
        )

if (USE_SDL_WINDOW)
  list(APPEND viewer_SOURCE_FILES
       llsdl.cpp
       llkeyboardsdl.cpp
       llwindowsdl.cpp
       )
  list(APPEND viewer_HEADER_FILES
       llsdl.h
       llkeyboardsdl.h
       llwindowsdl.h
       )

  list(APPEND llwindow_LINK_LIBRARIES
       ll::SDL3
       )
endif ()

if (DARWIN)
  list(APPEND llwindow_SOURCE_FILES
    llkeyboardmacosx.cpp
    llwindowmacosx.cpp
    llwindowmacosx-objc.mm
    llopenglview-objc.mm
    )
  list(APPEND llwindow_HEADER_FILES
    llkeyboardmacosx.h
    llwindowmacosx.h
    llwindowmacosx-objc.h
    llopenglview-objc.h
    llappdelegate-objc.h
    )

  # We use a bunch of deprecated system APIs.
  set_source_files_properties(
      llkeyboardmacosx.cpp
      llwindowmacosx.cpp
      PROPERTIES
      COMPILE_FLAGS "-fpascal-strings"
      )

  set_source_files_properties(
    llwindowmacosx-objc.mm
    llopenglview-objc.mm
    PROPERTIES
    SKIP_PRECOMPILE_HEADERS TRUE
  )
endif (DARWIN)


if (WINDOWS)
  list(APPEND llwindow_SOURCE_FILES
       llwindowwin32.cpp
       lldxhardware.cpp
       llkeyboardwin32.cpp
       lldragdropwin32.cpp
       )
  list(APPEND llwindow_HEADER_FILES
       llwindowwin32.h
       lldxhardware.h
       llkeyboardwin32.h
       lldragdropwin32.h
       )
  list(APPEND llwindow_LINK_LIBRARIES
       comdlg32     # Common Dialogs for ChooseColor
       ole32
       dxgi
       d3d9
       )
endif (WINDOWS)


if (BUILD_HEADLESS)
  set(llwindowheadless_SOURCE_FILES
      llcursortypes.cpp
      llkeyboard.cpp
      llkeyboardheadless.cpp
      llwindowheadless.cpp
      llwindowcallbacks.cpp
      llwindow.cpp
      llwindowmesaheadless.cpp
      llmousehandler.cpp
      )

  set(llwindowheadless_HEADER_FILES
      llcursortypes.h
      llkeyboard.h
      llkeyboardheadless.h
      llwindowheadless.h
      llwindowcallbacks.h
      llwindowmesaheadless.h
      llmousehandler.h
      )

  add_library (llwindowheadless
    ${llwindowheadless_SOURCE_FILES}
    ${llwindowheadless_HEADER_FILES}
    )

  target_compile_definitions( llwindowheadless PUBLIC LL_MESA=1 LL_MESA_HEADLESS=1 )

  target_link_libraries (llwindowheadless
  llcommon
  llimage
  llmath
  llrenderheadless
  llfilesystem
  llxml
  ll::glm
  ll::glext
  ll::zlib-ng
  PkgConfig::OSMESA
  fontconfig
  dl
  )
  target_include_directories(llwindowheadless INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
  
  if (USE_PRECOMPILED_HEADERS)
    target_precompile_headers(llwindowheadless REUSE_FROM llprecompiled)
  endif ()
endif (BUILD_HEADLESS)

if (llwindow_HEADER_FILES)
  list(APPEND llwindow_SOURCE_FILES ${llwindow_HEADER_FILES})
endif (llwindow_HEADER_FILES)

  list(APPEND viewer_SOURCE_FILES ${viewer_HEADER_FILES})

  add_library (llwindow
    ${llwindow_SOURCE_FILES}
    ${viewer_SOURCE_FILES}
    )

target_link_libraries (llwindow ${llwindow_LINK_LIBRARIES})
target_include_directories(llwindow INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

if (USE_SDL_WINDOW)
  target_compile_definitions( llwindow PUBLIC LL_SDL_WINDOW=1 )
endif()

if (USE_PRECOMPILED_HEADERS)
  target_precompile_headers(llwindow REUSE_FROM llprecompiled)
endif ()

if (DARWIN)
  target_link_libraries(llwindow ${CARBON_LIBRARY})
endif (DARWIN)
