
include(PluginAPI)

### SLPlugin

add_executable(SLPlugin
    WIN32
    MACOSX_BUNDLE
    )

target_sources(SLPlugin
    PRIVATE
    slplugin.cpp
    )

if (DARWIN)
  target_sources(SLPlugin
    PRIVATE
    slplugin-objc.mm

    PUBLIC
    FILE_SET HEADERS
    FILES
    slplugin-objc.h
  )

  set_source_files_properties(
    slplugin-objc.mm
    PROPERTIES
    SKIP_PRECOMPILE_HEADERS TRUE
  )
endif (DARWIN)

target_link_libraries(SLPlugin
        llplugin
        llmessage
        llcommon
        ll::pluginlibraries
        )

if (USE_PRECOMPILED_HEADERS)
  target_precompile_headers(SLPlugin REUSE_FROM llprecompiled_exe)
endif ()

if (WINDOWS)
  set_target_properties(SLPlugin
    PROPERTIES
    LINK_FLAGS_DEBUG "/NODEFAULTLIB:\"LIBCMTD\""
    )
elseif (DARWIN)
  set_target_properties(SLPlugin
    PROPERTIES
    BUILD_WITH_INSTALL_RPATH 1
    INSTALL_RPATH "@executable_path/../../../../Frameworks;@executable_path/../Frameworks;@executable_path/../Frameworks/plugins"
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/slplugin_info.plist
    XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym"
  )

  # Make sure the app bundle has a Resources directory (it will get populated by viewer-manifest.py later)
  add_custom_command(
    TARGET SLPlugin POST_BUILD
    COMMAND mkdir
    ARGS
      -p
      ${CMAKE_CURRENT_BINARY_DIR}/$<IF:$<BOOL:${LL_GENERATOR_IS_MULTI_CONFIG}>,$<CONFIG>,>/SLPlugin.app/Contents/Resources
  )
endif ()
