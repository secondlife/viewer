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
include(LLCommon)
include(LLImage)
include(LLWindow)
include(UI)
include(ViewerMiscLibs)
include(GLM)

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

# Platform-specific files for IDE visibility (excluded from build on other platforms)
set(macosx_SOURCE_FILES
    llkeyboardmacosx.cpp
    llwindowmacosx.cpp
    llwindowmacosx-objc.mm
    llopenglview-objc.mm
    )

set(macosx_HEADER_FILES
    llkeyboardmacosx.h
    llwindowmacosx.h
    llwindowmacosx-objc.h
    llwindowmacosx_iokit.h
    llopenglview-objc.h
    llappdelegate-objc.h
    )

set(linux_SOURCE_FILES
    llkeyboardsdl.cpp
    llwindowsdl.cpp
    )

set(linux_HEADER_FILES
    llkeyboardsdl.h
    llwindowsdl.h
    )

set(windows_SOURCE_FILES
    llwindowwin32.cpp
    lldxhardware.cpp
    llkeyboardwin32.cpp
    lldragdropwin32.cpp
    )

set(windows_HEADER_FILES
    llwindowwin32.h
    lldxhardware.h
    llkeyboardwin32.h
    lldragdropwin32.h
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
        ll::SDL
        )
# Libraries on which this library depends, needed for Linux builds
# Sort by high-level to low-level
if (LINUX)
  list(APPEND viewer_SOURCE_FILES ${linux_SOURCE_FILES})
  list(APPEND viewer_HEADER_FILES ${linux_HEADER_FILES})

  if (BUILD_HEADLESS)
    set(llwindowheadless_LINK_LIBRARIES
        ${LLCOMMON_LIBRARIES}
        ${LLIMAGE_LIBRARIES}
        ${LLMATH_LIBRARIES}
        ${LLRENDER_HEADLESS_LIBRARIES}
        ${LLFILESYSTEM_LIBRARIES}
        ${LLWINDOW_HEADLESS_LIBRARIES}
        ${LLXML_LIBRARIES}
        fontconfig          # For FCInit and other FC* functions.
        )
  endif (BUILD_HEADLESS)

endif (LINUX)

if (DARWIN)
  list(APPEND llwindow_SOURCE_FILES ${macosx_SOURCE_FILES})
  list(APPEND llwindow_HEADER_FILES ${macosx_HEADER_FILES})

  # We use a bunch of deprecated system APIs.
  set_source_files_properties(
      llkeyboardmacosx.cpp
      llwindowmacosx.cpp
      PROPERTIES
      COMPILE_FLAGS "-fpascal-strings"
      )
endif (DARWIN)


if (WINDOWS)
  list(APPEND llwindow_SOURCE_FILES ${windows_SOURCE_FILES})
  list(APPEND llwindow_HEADER_FILES ${windows_HEADER_FILES})
  list(APPEND llwindow_LINK_LIBRARIES
       comdlg32     # Common Dialogs for ChooseColor
       ole32
       dxgi
       d3d9
       )
endif (WINDOWS)

if (SOLARIS)
  list(APPEND llwindow_SOURCE_FILES
       llwindowsolaris.cpp
       )
  list(APPEND llwindow_HEADER_FILES
       llwindowsolaris.h
       )
endif (SOLARIS)

if (BUILD_HEADLESS)
  set(llwindowheadless_SOURCE_FILES
       llwindowmesaheadless.cpp
       llmousehandler.cpp
       )
  set(llwindowheadless_HEADER_FILES
       llwindowmesaheadless.h
       llmousehandler.h
    )
  add_library (llwindowheadless
    ${llwindow_SOURCE_FILES}
    ${llwindowheadless_SOURCE_FILES}
    )
  set_property(TARGET llwindowheadless
    PROPERTY COMPILE_DEFINITIONS LL_MESA=1 LL_MESA_HEADLESS=1
    )
  target_link_libraries (llwindowheadless ${llwindowheadless_LINK_LIBRARIES} dl)
endif (BUILD_HEADLESS)

if (llwindow_HEADER_FILES)
  list(APPEND llwindow_SOURCE_FILES ${llwindow_HEADER_FILES})
endif (llwindow_HEADER_FILES)

  list(APPEND viewer_SOURCE_FILES ${viewer_HEADER_FILES})

  # Collect all platform files for IDE visibility, excluding already-added current platform
  set(ide_visibility_files "")
  if (NOT DARWIN)
    list(APPEND ide_visibility_files ${macosx_SOURCE_FILES} ${macosx_HEADER_FILES})
  endif()
  if (NOT LINUX)
    list(APPEND ide_visibility_files ${linux_SOURCE_FILES} ${linux_HEADER_FILES})
  endif()
  if (NOT WINDOWS)
    list(APPEND ide_visibility_files ${windows_SOURCE_FILES} ${windows_HEADER_FILES})
  endif()

  add_library (llwindow
    ${llwindow_SOURCE_FILES}
    ${viewer_SOURCE_FILES}
    # Add non-platform files for IDE visibility only
    ${ide_visibility_files}
    )

# Mark non-platform files as excluded from build
if (WINDOWS)
  set_source_files_properties(
    ${macosx_SOURCE_FILES}
    ${linux_SOURCE_FILES}
    PROPERTIES
    HEADER_FILE_ONLY TRUE
    )
elseif (DARWIN)
  set_source_files_properties(
    ${windows_SOURCE_FILES}
    ${linux_SOURCE_FILES}
    PROPERTIES
    HEADER_FILE_ONLY TRUE
    )
elseif (LINUX)
  set_source_files_properties(
    ${macosx_SOURCE_FILES}
    ${windows_SOURCE_FILES}
    PROPERTIES
    HEADER_FILE_ONLY TRUE
    )
endif()

if (SDL_FOUND)
  set_property(TARGET llwindow
    PROPERTY COMPILE_DEFINITIONS LL_SDL=1
    )
endif (SDL_FOUND)

  target_link_libraries (llwindow ${llwindow_LINK_LIBRARIES})
  target_include_directories(llwindow INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

if (DARWIN)
  target_link_libraries(llwindow ${CARBON_LIBRARY})
endif (DARWIN)
