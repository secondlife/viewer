# -*- cmake -*-

project(media_plugin_base)

include(00-Common)
include(LLCommon)
include(LLImage)
include(LLWindow)
include(Linking)
include(PluginAPI)


### media_plugin_base


set(media_plugin_base_SOURCE_FILES
    media_plugin_base.cpp
)

set(media_plugin_base_HEADER_FILES
    CMakeLists.txt

    media_plugin_base.h
)

add_library(media_plugin_base
    ${media_plugin_base_SOURCE_FILES}
        )

target_link_libraries( media_plugin_base llplugin )
target_include_directories( media_plugin_base  INTERFACE   ${CMAKE_CURRENT_SOURCE_DIR})
