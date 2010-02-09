# -*- cmake -*-

project(media_plugin_webkit)

include(00-Common)
include(LLCommon)
include(LLImage)
include(LLPlugin)
include(LLMath)
include(LLRender)
include(LLWindow)
include(LLUI)
include(Linking)
include(PluginAPI)
include(MediaPluginBase)
include(FindOpenGL)

include(WebKitLibPlugin)

include_directories(
    ${LLPLUGIN_INCLUDE_DIRS}
    ${MEDIA_PLUGIN_BASE_INCLUDE_DIRS}
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
    ${LLIMAGE_INCLUDE_DIRS}
    ${LLRENDER_INCLUDE_DIRS}
    ${LLWINDOW_INCLUDE_DIRS}
)


### media_plugin_webkit

set(media_plugin_webkit_SOURCE_FILES
    media_plugin_webkit.cpp
    )

add_library(media_plugin_webkit
    SHARED
    ${media_plugin_webkit_SOURCE_FILES}
)

set(media_plugin_webkit_LINK_LIBRARIES
  ${LLPLUGIN_LIBRARIES}
  ${MEDIA_PLUGIN_BASE_LIBRARIES}
  ${LLCOMMON_LIBRARIES}
  ${WEBKIT_PLUGIN_LIBRARIES}
  ${PLUGIN_API_WINDOWS_LIBRARIES}
)

if (LINUX)
  list(APPEND media_plugin_webkit_LINK_LIBRARIES
       ${UI_LIBRARIES}     # for glib/GTK
       )
endif (LINUX)

target_link_libraries(media_plugin_webkit ${media_plugin_webkit_LINK_LIBRARIES})

add_dependencies(media_plugin_webkit
  ${LLPLUGIN_LIBRARIES}
  ${MEDIA_PLUGIN_BASE_LIBRARIES}
  ${LLCOMMON_LIBRARIES}
)

if (WINDOWS)
  set_target_properties(
    media_plugin_webkit
    PROPERTIES
    LINK_FLAGS "/MANIFEST:NO"
    )
endif (WINDOWS)

if (DARWIN)
  # Don't prepend 'lib' to the executable name, and don't embed a full path in the library's install name
  set_target_properties(
    media_plugin_webkit
    PROPERTIES
    PREFIX ""
    BUILD_WITH_INSTALL_RPATH 1
    INSTALL_NAME_DIR "@executable_path"
    LINK_FLAGS "-exported_symbols_list ${CMAKE_CURRENT_SOURCE_DIR}/../base/media_plugin_base.exp"
  )

  # copy the webkit dylib to the build directory
  add_custom_command(
    TARGET media_plugin_webkit POST_BUILD
#    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/libllqtwebkit.dylib
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/../libraries/universal-darwin/lib_release/libllqtwebkit.dylib ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/
    DEPENDS media_plugin_webkit ${CMAKE_SOURCE_DIR}/../libraries/universal-darwin/lib_release/libllqtwebkit.dylib
    )
    
endif (DARWIN)

