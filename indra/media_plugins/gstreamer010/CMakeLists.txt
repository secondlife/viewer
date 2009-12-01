# -*- cmake -*-

project(media_plugin_gstreamer010)

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
include(FindOpenGL)

include(GStreamer010Plugin)

include_directories(
    ${LLPLUGIN_INCLUDE_DIRS}
    ${MEDIA_PLUGIN_BASE_INCLUDE_DIRS}
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
    ${LLIMAGE_INCLUDE_DIRS}
    ${LLRENDER_INCLUDE_DIRS}
    ${LLWINDOW_INCLUDE_DIRS}
    ${GSTREAMER010_INCLUDE_DIRS}
    ${GSTREAMER010_PLUGINS_BASE_INCLUDE_DIRS}
)

### media_plugin_gstreamer010

set(media_plugin_gstreamer010_SOURCE_FILES
    media_plugin_gstreamer010.cpp
    llmediaimplgstreamer_syms.cpp
    llmediaimplgstreamervidplug.cpp
    )

set(media_plugin_gstreamer010_HEADER_FILES
    llmediaimplgstreamervidplug.h
    llmediaimplgstreamer_syms.h
    llmediaimplgstreamertriviallogging.h
    )

if (${CXX_VERSION_NUMBER} MATCHES "4[23].")
    # Work around a bad interaction between broken gstreamer headers and
    # g++ 4.3's increased strictness.
    set_source_files_properties(llmediaimplgstreamervidplug.cpp PROPERTIES
                                COMPILE_FLAGS -Wno-write-strings)
endif (${CXX_VERSION_NUMBER} MATCHES "4[23].")

add_library(media_plugin_gstreamer010
    SHARED
    ${media_plugin_gstreamer010_SOURCE_FILES}
)

target_link_libraries(media_plugin_gstreamer010
  ${LLPLUGIN_LIBRARIES}
  ${MEDIA_PLUGIN_BASE_LIBRARIES}
  ${LLCOMMON_LIBRARIES}
  ${PLUGIN_API_WINDOWS_LIBRARIES}
  ${GSTREAMER010_LIBRARIES}
)

add_dependencies(media_plugin_gstreamer010
  ${LLPLUGIN_LIBRARIES}
  ${MEDIA_PLUGIN_BASE_LIBRARIES}
  ${LLCOMMON_LIBRARIES}
)


