project(SLPlugin)

include(00-Common)
include(LLCommon)
include(LLPlugin)
include(Linking)
include(PluginAPI)
include(LLMessage)

include_directories(
    ${LLPLUGIN_INCLUDE_DIRS}
    ${LLMESSAGE_INCLUDE_DIRS}
    ${LLCOMMON_INCLUDE_DIRS}
)
include_directories(SYSTEM
    ${LLCOMMON_SYSTEM_INCLUDE_DIRS}
    )

if (DARWIN)
    include(CMakeFindFrameworks)
    find_library(COCOA_LIBRARY Cocoa)
endif (DARWIN)


### SLPlugin

set(SLPlugin_SOURCE_FILES
    slplugin.cpp
    )

if (DARWIN)
  list(APPEND SLPlugin_SOURCE_FILES
    slplugin-objc.mm
  )
  list(APPEND SLPlugin_HEADER_FILES
    slplugin-objc.h
  )
endif (DARWIN)

set_source_files_properties(${SLPlugin_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

if (SLPlugin_HEADER_FILES)
  list(APPEND SLPlugin_SOURCE_FILES ${SLPlugin_HEADER_FILES})
endif (SLPlugin_HEADER_FILES)

add_executable(SLPlugin
    WIN32
    MACOSX_BUNDLE
    ${SLPlugin_SOURCE_FILES}
)

set_target_properties(SLPlugin
  PROPERTIES
  MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/slplugin_info.plist
  )

target_link_libraries(SLPlugin
  ${LLPLUGIN_LIBRARIES}
  ${LLMESSAGE_LIBRARIES}
  ${LLCOMMON_LIBRARIES}
  ${PLUGIN_API_WINDOWS_LIBRARIES}
)

add_dependencies(SLPlugin
  ${LLPLUGIN_LIBRARIES}
  ${LLMESSAGE_LIBRARIES}
  ${LLCOMMON_LIBRARIES}
)

if (DARWIN)
  # Mac version needs to link against Carbon
  target_link_libraries(SLPlugin ${COCOA_LIBRARY})
  # Make sure the app bundle has a Resources directory (it will get populated by viewer-manifest.py later)
  add_custom_command(
    TARGET SLPlugin POST_BUILD
    COMMAND mkdir
    ARGS
      -p
      ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/SLPlugin.app/Contents/Resources
  )
endif (DARWIN)

if (LL_TESTS)
  ll_deploy_sharedlibs_command(SLPlugin)
endif (LL_TESTS)
