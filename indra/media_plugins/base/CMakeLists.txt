# -*- cmake -*-
include(PluginAPI)

### media_plugin_base
set(media_plugin_base_SOURCE_FILES
    media_plugin_base.cpp
)

set(media_plugin_base_HEADER_FILES
    CMakeLists.txt

    media_plugin_base.h
)

add_library(media_plugin_base STATIC
    ${media_plugin_base_SOURCE_FILES}
        )

target_link_libraries( media_plugin_base llplugin ll::pluginlibraries)
target_include_directories( media_plugin_base  INTERFACE   ${CMAKE_CURRENT_SOURCE_DIR})
