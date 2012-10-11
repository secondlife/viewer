# -*- cmake -*-

project(media_plugin_webkit)

include(00-Common)
include(LLCommon)
include(LLImage)
include(LLPlugin)
include(LLMath)
include(LLRender)
include(LLWindow)
include(UI)
include(Linking)
include(PluginAPI)
include(MediaPluginBase)
include(OpenGL)
include(PulseAudio)

include(WebKitLibPlugin)

include_directories(
    ${PULSEAUDIO_INCLUDE_DIRS}
    ${LLPLUGIN_INCLUDE_DIRS}
    ${MEDIA_PLUGIN_BASE_INCLUDE_DIRS}
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
    ${LLIMAGE_INCLUDE_DIRS}
    ${LLRENDER_INCLUDE_DIRS}
    ${LLWINDOW_INCLUDE_DIRS}
    ${LLQTWEBKIT_INCLUDE_DIR}
)
include_directories(SYSTEM
    ${LLCOMMON_SYSTEM_INCLUDE_DIRS}
    )


### media_plugin_webkit

if(NOT WORD_SIZE EQUAL 32)
  if(WINDOWS)
    add_definitions(/FIXED:NO)
  else(WINDOWS) # not windows therefore gcc LINUX and DARWIN
    add_definitions(-fPIC)
  endif(WINDOWS)
endif(NOT WORD_SIZE EQUAL 32)

set(media_plugin_webkit_SOURCE_FILES
    media_plugin_webkit.cpp
    )

set(media_plugin_webkit_HEADER_FILES
    volume_catcher.h
    )

set(media_plugin_webkit_LINK_LIBRARIES
  ${LLPLUGIN_LIBRARIES}
  ${MEDIA_PLUGIN_BASE_LIBRARIES}
  ${LLCOMMON_LIBRARIES}
  ${WEBKIT_PLUGIN_LIBRARIES}
  ${PLUGIN_API_WINDOWS_LIBRARIES}
  ${PULSEAUDIO_LIBRARIES}
)

# Select which VolumeCatcher implementation to use
if (LINUX)
  if (PULSEAUDIO_FOUND)
    list(APPEND media_plugin_webkit_SOURCE_FILES linux_volume_catcher.cpp)
  else (PULSEAUDIO_FOUND)
    list(APPEND media_plugin_webkit_SOURCE_FILES dummy_volume_catcher.cpp)
  endif (PULSEAUDIO_FOUND)
  list(APPEND media_plugin_webkit_LINK_LIBRARIES
       ${UI_LIBRARIES}     # for glib/GTK
       )
elseif (DARWIN)
  list(APPEND media_plugin_webkit_SOURCE_FILES mac_volume_catcher.cpp)
  find_library(CORESERVICES_LIBRARY CoreServices)
  find_library(AUDIOUNIT_LIBRARY AudioUnit)
  list(APPEND media_plugin_webkit_LINK_LIBRARIES
       ${CORESERVICES_LIBRARY}     # for Component Manager calls
       ${AUDIOUNIT_LIBRARY}        # for AudioUnit calls
       )
elseif (WINDOWS)
  list(APPEND media_plugin_webkit_SOURCE_FILES windows_volume_catcher.cpp)
endif (LINUX)

set_source_files_properties(${media_plugin_webkit_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND media_plugin_webkit_SOURCE_FILES ${media_plugin_webkit_HEADER_FILES})

add_library(media_plugin_webkit
    SHARED
    ${media_plugin_webkit_SOURCE_FILES}
)

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
    COMMAND ${CMAKE_COMMAND} -E copy ${ARCH_PREBUILT_DIRS_RELEASE}/libllqtwebkit.dylib ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/
    DEPENDS media_plugin_webkit ${ARCH_PREBUILT_DIRS_RELEASE}/libllqtwebkit.dylib
    )
    
endif (DARWIN)

