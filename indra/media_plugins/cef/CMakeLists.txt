# -*- cmake -*-

project(media_plugin_cef)

include(Boost)
include(00-Common)
include(LLCommon)
include(LLWindow)
include(Linking)
include(PluginAPI)

include(CEFPlugin)
include(GLIB)

### media_plugin_cef

set(media_plugin_cef_SOURCE_FILES
    media_plugin_cef.cpp
    )

set(media_plugin_cef_HEADER_FILES
    volume_catcher.h
    )

# Select which VolumeCatcher implementation to use
if (LINUX)
  foreach( PULSE_FILE pulse/introspect.h pulse/context.h pulse/subscribe.h pulse/glib-mainloop.h )
    find_path( PULSE_FILE_${PULSE_FILE}_FOUND ${PULSE_FILE} NO_CACHE)
    if( NOT PULSE_FILE_${PULSE_FILE}_FOUND )
      message( "Looking for ${PULSE_FILE} ... not found")
      message( FATAL_ERROR "Pulse header not found" )
    else()
      message( "Looking for ${PULSE_FILE} ... found")
    endif()
  endforeach()
  
  include(FindPipeWire)
  include_directories(SYSTEM ${PIPEWIRE_INCLUDE_DIRS} ${SPA_INCLUDE_DIRS})
  
  message( "Building with Linux volume catcher for PipeWire and PulseAudio" )

  list(APPEND media_plugin_cef_HEADER_FILES
    linux/volume_catcher_linux.h
  )

  set(LINUX_VOLUME_CATCHER 
    linux/volume_catcher_linux.cpp 
    linux/volume_catcher_pulseaudio.cpp
    linux/volume_catcher_pipewire.cpp 
  )

  list(APPEND media_plugin_cef_SOURCE_FILES ${LINUX_VOLUME_CATCHER})
  set(CMAKE_SHARED_LINKER_FLAGS  "-Wl,--build-id -Wl,-rpath,'$ORIGIN:$ORIGIN/../../lib'")
  list(APPEND media_plugin_cef_LINK_LIBRARIES llwindow )
elseif (DARWIN)
  list(APPEND media_plugin_cef_SOURCE_FILES volume_catcher_null.cpp)
  find_library(CORESERVICES_LIBRARY CoreServices)
  find_library(AUDIOUNIT_LIBRARY AudioUnit)
  set( media_plugin_cef_LINK_LIBRARIES
       ${CORESERVICES_LIBRARY}     # for Component Manager calls
       ${AUDIOUNIT_LIBRARY}        # for AudioUnit calls
       )
elseif (WINDOWS)
  list(APPEND media_plugin_cef_SOURCE_FILES windows_volume_catcher.cpp)
endif (LINUX)

list(APPEND media_plugin_cef_SOURCE_FILES ${media_plugin_cef_HEADER_FILES})

add_library(media_plugin_cef
    SHARED
    ${media_plugin_cef_SOURCE_FILES}
    )

#add_dependencies(media_plugin_cef
#  ${MEDIA_PLUGIN_BASE_LIBRARIES}
#)

target_link_libraries(media_plugin_cef
        media_plugin_base
        ll::cef
        ll::glib_headers
)

if (WINDOWS)
  set_target_properties(
    media_plugin_cef
    PROPERTIES
    LINK_FLAGS "/MANIFEST:NO /NODEFAULTLIB:LIBCMT /IGNORE:4099"
    )
endif (WINDOWS)

if (DARWIN)
  # Don't prepend 'lib' to the executable name, and don't embed a full path in the library's install name
  set_target_properties(
    media_plugin_cef
    PROPERTIES
    PREFIX ""
    BUILD_WITH_INSTALL_RPATH 1
    INSTALL_NAME_DIR "@executable_path"
    LINK_FLAGS "-exported_symbols_list ${CMAKE_CURRENT_SOURCE_DIR}/../base/media_plugin_base.exp"
  )

  add_custom_command(TARGET media_plugin_cef
    POST_BUILD COMMAND ${CMAKE_INSTALL_NAME_TOOL} -change "@executable_path/Chromium Embedded Framework"
        "@executable_path/../../../../Frameworks/Chromium Embedded Framework.framework/Chromium Embedded Framework"
        "$<TARGET_FILE:media_plugin_cef>"
    VERBATIM
    COMMENT "Fixing path to CEF Framework"
  )

endif (DARWIN)
