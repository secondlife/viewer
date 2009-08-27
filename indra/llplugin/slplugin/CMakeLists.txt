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
    ${SLPlugin_SOURCE_FILES}
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
  # Mac version needs to link against carbon, and also needs an embedded plist (to set LSBackgroundOnly)
  target_link_libraries(SLPlugin ${CARBON_LIBRARY})
  set_target_properties(
    SLPlugin
    PROPERTIES
    LINK_FLAGS "-Wl,-sectcreate,__TEXT,__info_plist,${CMAKE_CURRENT_SOURCE_DIR}/slplugin_info.plist"
  )
endif (DARWIN)

