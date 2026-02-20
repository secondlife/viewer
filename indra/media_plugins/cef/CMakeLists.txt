# -*- cmake -*-
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
if (FALSE) # Disable linux volume catcher due to issues dlopening with CEF 139
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
elseif (WINDOWS)
  list(APPEND media_plugin_cef_SOURCE_FILES windows_volume_catcher.cpp)
else()
  list(APPEND media_plugin_cef_SOURCE_FILES volume_catcher_null.cpp)
endif ()

list(APPEND media_plugin_cef_SOURCE_FILES ${media_plugin_cef_HEADER_FILES})

add_library(media_plugin_cef
    SHARED
    ${media_plugin_cef_SOURCE_FILES}
    )

target_link_libraries(media_plugin_cef
        media_plugin_base
        ll::cef
        ll::glib_headers
)

if (WINDOWS)
  set_target_properties(media_plugin_cef
    PROPERTIES
    EXCLUDE_FROM_DEFAULT_BUILD_DEBUG ON
    RUNTIME_OUTPUT_DIRECTORY "${VIEWER_STAGING_DIR}/llplugin"
  )
  target_link_options(media_plugin_cef PRIVATE /MANIFEST:NO /IGNORE:4099)

  # Copy plugin files to packaging directory
  set(DULLAHAN_DIR "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/dullahan-bin")
  add_custom_command(
    TARGET media_plugin_cef POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${DULLAHAN_DIR}/bin" "${VIEWER_STAGING_DIR}/llplugin"
  )

  set(CEF_RESOURCE_FILES
    "${DULLAHAN_DIR}/resources/chrome_100_percent.pak"
    "${DULLAHAN_DIR}/resources/chrome_200_percent.pak"
    "${DULLAHAN_DIR}/resources/resources.pak"
    "${DULLAHAN_DIR}/resources/icudtl.dat"
  )
  add_custom_command(
    TARGET media_plugin_cef POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CEF_RESOURCE_FILES} "${VIEWER_STAGING_DIR}/llplugin"
    COMMAND_EXPAND_LISTS
  )

  add_custom_command(
    TARGET media_plugin_cef POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/dullahan-bin/resources/locales" "${VIEWER_STAGING_DIR}/llplugin/locales"
  )

elseif (DARWIN)
    find_library(CORESERVICES_LIBRARY CoreServices)
    find_library(AUDIOUNIT_LIBRARY AudioUnit)

    target_link_libraries(media_plugin_cef
    ${CORESERVICES_LIBRARY}     # for Component Manager calls
    ${AUDIOUNIT_LIBRARY}        # for AudioUnit calls
    )

  # Don't prepend 'lib' to the executable name, and don't embed a full path in the library's install name
  set_target_properties(
    media_plugin_cef
    PROPERTIES
    PREFIX ""
    BUILD_WITH_INSTALL_RPATH 1
    INSTALL_RPATH "@executable_path/../Frameworks"
  )

elseif (LINUX)
  target_link_options(media_plugin_cef PRIVATE  "LINKER:--build-id" "LINKER:-rpath,'$ORIGIN:$ORIGIN/../../lib'")
endif ()
