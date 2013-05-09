# -*- cmake -*-

project(media_plugin_quicktime)

include(00-Common)
include(LLCommon)
include(LLImage)
include(LLPlugin)
include(LLMath)
include(LLRender)
include(LLWindow)
include(Linking)
include(PluginAPI)
include(MediaPluginBase)
include(OpenGL)
include(QuickTimePlugin)

include_directories(
    ${LLPLUGIN_INCLUDE_DIRS}
    ${MEDIA_PLUGIN_BASE_INCLUDE_DIRS}
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
    ${LLIMAGE_INCLUDE_DIRS}
    ${LLRENDER_INCLUDE_DIRS}
    ${LLWINDOW_INCLUDE_DIRS}
)
include_directories(SYSTEM
    ${LLCOMMON_SYSTEM_INCLUDE_DIRS}
    )

if (DARWIN)
    include(CMakeFindFrameworks)
    find_library(CARBON_LIBRARY Carbon)
endif (DARWIN)


### media_plugin_quicktime

set(media_plugin_quicktime_SOURCE_FILES
    media_plugin_quicktime.cpp
    )

add_library(media_plugin_quicktime
    SHARED
    ${media_plugin_quicktime_SOURCE_FILES}
)

target_link_libraries(media_plugin_quicktime
  ${LLPLUGIN_LIBRARIES}
  ${MEDIA_PLUGIN_BASE_LIBRARIES}
  ${LLCOMMON_LIBRARIES}
  ${QUICKTIME_LIBRARY}
  ${PLUGIN_API_WINDOWS_LIBRARIES}
)

add_dependencies(media_plugin_quicktime
  ${LLPLUGIN_LIBRARIES}
  ${MEDIA_PLUGIN_BASE_LIBRARIES}
  ${LLCOMMON_LIBRARIES}
)

if (WINDOWS)
  set_target_properties(
    media_plugin_quicktime
    PROPERTIES
    LINK_FLAGS "/MANIFEST:NO"
    )
endif (WINDOWS)

if (QUICKTIME)

    add_definitions(-DLL_QUICKTIME_ENABLED=1)

    if (DARWIN)
      # Don't prepend 'lib' to the executable name, and don't embed a full path in the library's install name
      set_target_properties(
        media_plugin_quicktime
        PROPERTIES
        PREFIX ""
        BUILD_WITH_INSTALL_RPATH 1
        INSTALL_NAME_DIR "@executable_path"
        LINK_FLAGS "-exported_symbols_list ${CMAKE_CURRENT_SOURCE_DIR}/../base/media_plugin_base.exp"
      )

# We use a bunch of deprecated system APIs.
    set_source_files_properties(
        media_plugin_quicktime.cpp PROPERTIES
        COMPILE_FLAGS -Wno-deprecated-declarations
        )
    find_library(CARBON_LIBRARY Carbon)
    target_link_libraries(media_plugin_quicktime ${CARBON_LIBRARY})
  endif (DARWIN)
endif (QUICKTIME)

