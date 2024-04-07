# -*- cmake -*-

project(media_plugin_gstreamer10)

include(00-Common)
include(LLCommon)
include(LLImage)
include(LLMath)
include(LLWindow)
include(Linking)
include(PluginAPI)
include(OpenGL)
include(GLIB)

include(GStreamer10Plugin)

### media_plugin_gstreamer10

set(media_plugin_gstreamer10_SOURCE_FILES
    media_plugin_gstreamer10.cpp
    )

set(media_plugin_gstreamer10_HEADER_FILES
    llmediaimplgstreamer_syms.h
    llmediaimplgstreamertriviallogging.h
    )

add_library(media_plugin_gstreamer10
    SHARED
    ${media_plugin_gstreamer10_SOURCE_FILES}
)

target_link_libraries(media_plugin_gstreamer10 media_plugin_base ll::gstreamer10 )

if (WINDOWS)
  set_target_properties(
    media_plugin_gstreamer10
    PROPERTIES
    LINK_FLAGS "/MANIFEST:NO /SAFESEH:NO /NODEFAULTLIB:LIBCMT"
    )
endif (WINDOWS)
