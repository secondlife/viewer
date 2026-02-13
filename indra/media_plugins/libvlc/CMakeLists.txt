# -*- cmake -*-

include(PluginAPI)
include(LibVLCPlugin)

### media_plugin_libvlc


set(media_plugin_libvlc_SOURCE_FILES
    media_plugin_libvlc.cpp
    )

add_library(media_plugin_libvlc
    SHARED
    ${media_plugin_libvlc_SOURCE_FILES}
    )

target_link_libraries(media_plugin_libvlc
        media_plugin_base
        ll::libvlc
)

if (WINDOWS)
  set_target_properties(media_plugin_libvlc
    PROPERTIES
    EXCLUDE_FROM_DEFAULT_BUILD_DEBUG ON
    RUNTIME_OUTPUT_DIRECTORY "${VIEWER_STAGING_DIR}/llplugin"
    )
  target_link_options(media_plugin_libvlc PRIVATE /MANIFEST:NO)

  # Copy plugin dlls to packaging directory
  add_custom_command(
    TARGET media_plugin_libvlc POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/plugins/vlc-bin/plugins" "${VIEWER_STAGING_DIR}/llplugin/plugins"
  )
elseif (DARWIN)
  # Don't prepend 'lib' to the executable name, and don't embed a full path in the library's install name
  set_target_properties(
    media_plugin_libvlc
    PROPERTIES
    PREFIX ""
    BUILD_WITH_INSTALL_RPATH 1
    INSTALL_RPATH "@executable_path/../Frameworks"
  )

endif ()
