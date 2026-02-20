# -*- cmake -*-

include(PluginAPI)

### media_plugin_example

set(media_plugin_example_SOURCE_FILES
    media_plugin_example.cpp
    )

add_library(media_plugin_example
    SHARED
    ${media_plugin_example_SOURCE_FILES}
    )

target_link_libraries(media_plugin_example media_plugin_base )

if (WINDOWS)
  set_target_properties(media_plugin_example
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${VIEWER_STAGING_DIR}/llplugin"
    )
  target_link_options(media_plugin_example PRIVATE /MANIFEST:NO)
elseif (DARWIN)
  # Don't prepend 'lib' to the executable name, and don't embed a full path in the library's install name
  set_target_properties(
    media_plugin_example
    PROPERTIES
    PREFIX ""
    BUILD_WITH_INSTALL_RPATH 1
    INSTALL_RPATH "@executable_path/../Frameworks"
  )

endif ()
