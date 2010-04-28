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

if (DARWIN)
    include(CMakeFindFrameworks)
    find_library(CARBON_LIBRARY Carbon)
endif (DARWIN)


### SLPlugin

set(SLPlugin_SOURCE_FILES
    slplugin.cpp
    )

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
  target_link_libraries(SLPlugin ${CARBON_LIBRARY})
  # Make sure the app bundle has a Resources directory (it will get populated by viewer-manifest.py later)
  add_custom_command(
    TARGET SLPlugin POST_BUILD
    COMMAND mkdir
    ARGS
      -p
      ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/SLPlugin.app/Contents/Resources
  )
endif (DARWIN)

ll_deploy_sharedlibs_command(SLPlugin)
