# -*- cmake -*-

project(llplugintest)

include(00-Common)
include(FindOpenGL)
include(LLCommon)
include(LLPlugin)
include(Linking)
include(PluginAPI)
include(LLImage)
include(LLMath)
include(LLMessage)
include(LLRender)
include(LLWindow)
include(Glut)
include(Glui)

include_directories(
    ${LLPLUGIN_INCLUDE_DIRS}
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLIMAGE_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
    ${LLMESSAGE_INCLUDE_DIRS}
    ${LLRENDER_INCLUDE_DIRS}
    ${LLWINDOW_INCLUDE_DIRS}
)

if (DARWIN)
    include(CMakeFindFrameworks)
    find_library(CARBON_LIBRARY Carbon)
endif (DARWIN)

### demo_plugin

#set(demo_plugin_SOURCE_FILES
#    demo_plugin.cpp
#    )
#
#add_library(demo_plugin
#    SHARED
#    ${demo_plugin_SOURCE_FILES}
#)
#
#target_link_libraries(demo_plugin
#  ${LLPLUGIN_LIBRARIES}
#  ${LLCOMMON_LIBRARIES}
#  ${PLUGIN_API_WINDOWS_LIBRARIES}
#)
#
#add_dependencies(demo_plugin
#  ${LLPLUGIN_LIBRARIES}
#  ${LLCOMMON_LIBRARIES}
#)
#
#if (DARWIN)
#  # Don't prepend 'lib' to the executable name, and don't embed a full path in the library's install name
#  set_target_properties(
#    demo_plugin
#    PROPERTIES
#    PREFIX ""
#    BUILD_WITH_INSTALL_RPATH 1
#    INSTALL_NAME_DIR "@executable_path"
#  )
#endif (DARWIN)

### plugin_host

#set(plugin_host_SOURCE_FILES
#    plugin_host.cpp
#    )
#
#add_executable(plugin_host
#    WIN32
#    ${plugin_host_SOURCE_FILES}
#)
#
#set_target_properties(plugin_host
#    PROPERTIES
#    WIN32_EXECUTABLE
#    FALSE
#)
#
#target_link_libraries(plugin_host
#  ${LLPLUGIN_LIBRARIES}
#  ${LLCOMMON_LIBRARIES}
#  ${PLUGIN_API_WINDOWS_LIBRARIES}
#)
#
#add_dependencies(plugin_host
#  demo_plugin
#  ${LLPLUGIN_LIBRARIES}
#  ${LLCOMMON_LIBRARIES}
#)

### plugin_process_launcher

#set(plugin_process_launcher_SOURCE_FILES
#    plugin_process_launcher.cpp
#    )
#
#add_executable(plugin_process_launcher
#    WIN32
#    ${plugin_process_launcher_SOURCE_FILES}
#)
#
#set_target_properties(plugin_process_launcher
#    PROPERTIES
#    WIN32_EXECUTABLE
#    FALSE
#)
#
#target_link_libraries(plugin_process_launcher
#  ${LLPLUGIN_LIBRARIES}
#  ${LLMESSAGE_LIBRARIES}
#  ${LLCOMMON_LIBRARIES}
#  ${PLUGIN_API_WINDOWS_LIBRARIES}
#)
#
#add_dependencies(plugin_process_launcher
#  SLPlugin
#  demo_plugin
#  ${LLPLUGIN_LIBRARIES}
#  ${LLMESSAGE_LIBRARIES}
#  ${LLCOMMON_LIBRARIES}
#)

### media_simple_test

#set(media_simple_test_SOURCE_FILES
#    media_simple_test.cpp
#    )
#
#add_executable(media_simple_test
#    WIN32
#    ${media_simple_test_SOURCE_FILES}
#)
#
#add_dependencies(media_simple_test stage_third_party_libs)
#
#set_target_properties(media_simple_test
#    PROPERTIES
#    WIN32_EXECUTABLE
#    FALSE
#)
#
#target_link_libraries(media_simple_test
#  ${GLUT_LIBRARY}
#  ${OPENGL_LIBRARIES}
#  ${LLCOMMON_LIBRARIES}
#)

### media_plugin_test

#set(media_plugin_test_SOURCE_FILES
#    media_plugin_test.cpp
#    )
#
#add_executable(media_plugin_test
#    WIN32
#    ${media_plugin_test_SOURCE_FILES}
#)
#
#set_target_properties(media_plugin_test
#    PROPERTIES
#    WIN32_EXECUTABLE
#    FALSE
#)
#
#target_link_libraries(media_plugin_test
#  ${GLUT_LIBRARY}
#  ${OPENGL_LIBRARIES}
#  ${LLPLUGIN_LIBRARIES}
#  ${LLMESSAGE_LIBRARIES}
#  ${LLCOMMON_LIBRARIES}
#  ${PLUGIN_API_WINDOWS_LIBRARIES}
#)
#
#add_dependencies(media_plugin_test
#  stage_third_party_libs
#  SLPlugin
#  demo_media_plugin
#  ${LLPLUGIN_LIBRARIES}
#  ${LLMESSAGE_LIBRARIES}
#  ${LLCOMMON_LIBRARIES}
#)

### demo_media_plugin

#set(demo_media_plugin_SOURCE_FILES
#    demo_media_plugin.cpp
#    )
#
#add_library(demo_media_plugin
#    SHARED
#    ${demo_media_plugin_SOURCE_FILES}
#)
#
#target_link_libraries(demo_media_plugin
#  ${LLPLUGIN_LIBRARIES}
#  ${LLCOMMON_LIBRARIES}
#  ${PLUGIN_API_WINDOWS_LIBRARIES}
#)
#
#add_dependencies(demo_media_plugin
#  ${LLPLUGIN_LIBRARIES}
#  ${LLCOMMON_LIBRARIES}
#)
#
#if (DARWIN)
#  # Don't prepend 'lib' to the executable name, and don't embed a full path in the library's install name
#  set_target_properties(
#    demo_media_plugin
#    PROPERTIES
#    PREFIX ""
#    BUILD_WITH_INSTALL_RPATH 1
#    INSTALL_NAME_DIR "@executable_path"
#  )
#endif (DARWIN)

### demo_media_plugin_2

#set(demo_media_plugin_2_SOURCE_FILES
#    demo_media_plugin_2.cpp
#    )
#
#add_library(demo_media_plugin_2
#    SHARED
#    ${demo_media_plugin_2_SOURCE_FILES}
#)
#
#target_link_libraries(demo_media_plugin_2
#  ${LLPLUGIN_LIBRARIES}
#  ${LLCOMMON_LIBRARIES}
#  ${PLUGIN_API_WINDOWS_LIBRARIES}
#)
#
#add_dependencies(demo_media_plugin_2
#  ${LLPLUGIN_LIBRARIES}
#  ${LLCOMMON_LIBRARIES}
#)
#
#if (DARWIN)
#  # Don't prepend 'lib' to the executable name, and don't embed a full path in the library's install name
#  set_target_properties(
#    demo_media_plugin_2
#    PROPERTIES
#    PREFIX ""
#    BUILD_WITH_INSTALL_RPATH 1
#    INSTALL_NAME_DIR "@executable_path"
#  )
#endif (DARWIN)

### llmediaplugintest

set(llmediaplugintest_SOURCE_FILES
    llmediaplugintest.cpp
    llmediaplugintest.h
    bookmarks.txt
    )

add_executable(llmediaplugintest
    WIN32
    ${llmediaplugintest_SOURCE_FILES}
)

set_target_properties(llmediaplugintest
    PROPERTIES
    WIN32_EXECUTABLE
    FALSE
)

target_link_libraries(llmediaplugintest
  ${GLUT_LIBRARY}
  ${GLUI_LIBRARY}
  ${OPENGL_LIBRARIES}
  ${LLPLUGIN_LIBRARIES}
  ${LLMESSAGE_LIBRARIES}
  ${LLCOMMON_LIBRARIES}
  ${PLUGIN_API_WINDOWS_LIBRARIES}
)

add_dependencies(llmediaplugintest
  stage_third_party_libs
  SLPlugin
  media_plugin_flash_activex
  media_plugin_quicktime
  media_plugin_webkit
  media_plugin_awesomium
  ${LLPLUGIN_LIBRARIES}
  ${LLMESSAGE_LIBRARIES}
  ${LLCOMMON_LIBRARIES}
)

# turn off weird GLUI pragma 
add_definitions(-DGLUI_NO_LIB_PRAGMA)

if (DARWIN OR LINUX)
  # glui.h contains code that triggers the "overloaded-virtual" warning in gcc.  
  set_source_files_properties(llmediaplugintest.cpp PROPERTIES COMPILE_FLAGS "-Wno-overloaded-virtual")
endif (DARWIN OR LINUX)

# Gather build products of the various dependencies into the build directory for the testbed.

get_target_property(BUILT_SLPLUGIN SLPlugin LOCATION)
add_custom_command(TARGET llmediaplugintest POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${BUILT_SLPLUGIN}  ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/
  DEPENDS ${BUILT_SLPLUGIN}
)

if (DARWIN OR WINDOWS)
  get_target_property(BUILT_WEBKIT_PLUGIN media_plugin_webkit LOCATION)
  add_custom_command(TARGET llmediaplugintest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${BUILT_WEBKIT_PLUGIN}  ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/
    DEPENDS ${BUILT_WEBKIT_PLUGIN}
  )

  get_target_property(BUILT_QUICKTIME_PLUGIN media_plugin_quicktime LOCATION)
  add_custom_command(TARGET llmediaplugintest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${BUILT_QUICKTIME_PLUGIN}  ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/
    DEPENDS ${BUILT_QUICKTIME_PLUGIN}
  )
  
  get_target_property(BUILT_AWESOMIUM_PLUGIN media_plugin_awesomium LOCATION)
  add_custom_command(TARGET llmediaplugintest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${BUILT_AWESOMIUM_PLUGIN}  ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/
    DEPENDS ${BUILT_AWESOMIUM_PLUGIN}
  )
  
  # copy over bookmarks file if llmediaplugintest gets built
  get_target_property(BUILT_LLMEDIAPLUGINTEST llmediaplugintest LOCATION)
  add_custom_command(TARGET llmediaplugintest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/bookmarks.txt ${CMAKE_CURRENT_BINARY_DIR}/
    DEPENDS ${BUILT_LLMEDIAPLUGINTEST}
  )
  # also copy it to the build configuration directory, which is what the mac wants...
  add_custom_command(TARGET llmediaplugintest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/bookmarks.txt ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/
    DEPENDS ${BUILT_LLMEDIAPLUGINTEST}
  )
endif (DARWIN OR WINDOWS)

if (WINDOWS)
  get_target_property(BUILT_FLASH_ACTIVEX_PLUGIN media_plugin_flash_activex LOCATION)
  add_custom_command(TARGET llmediaplugintest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${BUILT_FLASH_ACTIVEX_PLUGIN}  ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/
    DEPENDS ${BUILT_FLASH_ACTIVEX_PLUGIN}
  )
  
endif (WINDOWS)

if (DARWIN)
  add_custom_command(TARGET llmediaplugintest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/../libraries/universal-darwin/lib_release/libllwebkitlib.dylib ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/
    DEPENDS ${CMAKE_SOURCE_DIR}/../libraries/universal-darwin/lib_release/libllwebkitlib.dylib
  )

  add_custom_command(TARGET llmediaplugintest POST_BUILD
    COMMAND rsync -av --delete ${CMAKE_SOURCE_DIR}/../libraries/universal-darwin/lib_release/Awesomium.framework ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/
    DEPENDS ${CMAKE_SOURCE_DIR}/../libraries/universal-darwin/lib_release/Awesomium.framework
  )
endif (DARWIN)


