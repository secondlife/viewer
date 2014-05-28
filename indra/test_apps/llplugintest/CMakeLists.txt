# -*- cmake -*-
project(llplugintest)

include(00-Common)
include(OpenGL)
include(LLCommon)
include(LLPlugin)
include(Linking)
include(LLSharedLibs)
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
include_directories(SYSTEM
    ${LLCOMMON_SYSTEM_INCLUDE_DIRS}
    )

if (DARWIN)
    include(CMakeFindFrameworks)
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
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
    MACOSX_BUNDLE
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

if (DARWIN)
  # The testbed needs to use a couple of CoreFoundation calls now, to deal with being a bundled app.
  target_link_libraries(llmediaplugintest
    ${COREFOUNDATION_LIBRARY}
  )
endif (DARWIN)

add_dependencies(llmediaplugintest
  stage_third_party_libs
  SLPlugin
  media_plugin_quicktime
  media_plugin_webkit
  media_plugin_example
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

if (DARWIN)
  # path inside the app bundle where we'll need to copy plugins and other related files
  set(PLUGINS_DESTINATION_DIR
    ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/llmediaplugintest.app/Contents/Resources
  )
  
  # create the Contents/Resources directory
  add_custom_command(
    TARGET llmediaplugintest POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS
      -E
      make_directory
      ${PLUGINS_DESTINATION_DIR}
    COMMENT "Creating Resources directory in app bundle."
  ) 
else (DARWIN)
  set(PLUGINS_DESTINATION_DIR
    ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/
  )
endif (DARWIN)

get_target_property(BUILT_SLPLUGIN SLPlugin LOCATION)
add_custom_command(TARGET llmediaplugintest POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${BUILT_SLPLUGIN}  ${PLUGINS_DESTINATION_DIR}
  DEPENDS ${BUILT_SLPLUGIN}
)

get_target_property(BUILT_LLCOMMON llcommon LOCATION)
add_custom_command(TARGET llmediaplugintest POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${BUILT_LLCOMMON}  ${PLUGINS_DESTINATION_DIR}
  DEPENDS ${BUILT_LLCOMMON}
)


if (DARWIN OR WINDOWS)
  get_target_property(BUILT_WEBKIT_PLUGIN media_plugin_webkit LOCATION)
  add_custom_command(TARGET llmediaplugintest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${BUILT_WEBKIT_PLUGIN}  ${PLUGINS_DESTINATION_DIR}
    DEPENDS ${BUILT_WEBKIT_PLUGIN}
  )

  get_target_property(BUILT_QUICKTIME_PLUGIN media_plugin_quicktime LOCATION)
  add_custom_command(TARGET llmediaplugintest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${BUILT_QUICKTIME_PLUGIN}  ${PLUGINS_DESTINATION_DIR}
    DEPENDS ${BUILT_QUICKTIME_PLUGIN}
  )
  
  get_target_property(BUILT_EXAMPLE_PLUGIN media_plugin_example LOCATION)
  add_custom_command(TARGET llmediaplugintest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${BUILT_EXAMPLE_PLUGIN}  ${PLUGINS_DESTINATION_DIR}
    DEPENDS ${BUILT_EXAMPLE_PLUGIN}
  )
  
  # copy over bookmarks file if llmediaplugintest gets built
  get_target_property(BUILT_LLMEDIAPLUGINTEST llmediaplugintest LOCATION)
  add_custom_command(TARGET llmediaplugintest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/bookmarks.txt ${CMAKE_CURRENT_BINARY_DIR}/
    DEPENDS ${BUILT_LLMEDIAPLUGINTEST}
  )
  # also copy it to the same place as SLPlugin, which is what the mac wants...
  add_custom_command(TARGET llmediaplugintest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/bookmarks.txt ${PLUGINS_DESTINATION_DIR}
    DEPENDS ${BUILT_LLMEDIAPLUGINTEST}
  )
endif (DARWIN OR WINDOWS)

if (DARWIN)
  add_custom_command(TARGET llmediaplugintest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${ARCH_PREBUILT_DIRS_RELEASE}/libllqtwebkit.dylib ${PLUGINS_DESTINATION_DIR}
    DEPENDS ${ARCH_PREBUILT_DIRS_RELEASE}/libllqtwebkit.dylib
  )
endif (DARWIN)

if(WINDOWS)
  #********************
  # Plugin test library deploy
  #
  # Debug config runtime files required for the plugin test mule
  set(plugintest_debug_src_dir "${ARCH_PREBUILT_DIRS_DEBUG}")
  set(plugintest_debug_files
    libeay32.dll
    libglib-2.0-0.dll
    libgmodule-2.0-0.dll
    libgobject-2.0-0.dll
    libgthread-2.0-0.dll
    qtcored4.dll
    qtguid4.dll
    qtnetworkd4.dll
    qtopengld4.dll
    qtwebkitd4.dll
    ssleay32.dll
    )
  copy_if_different(
    ${plugintest_debug_src_dir}
    "${CMAKE_CURRENT_BINARY_DIR}/Debug"
    out_targets
    ${plugintest_debug_files}
    )
  set(plugin_test_targets ${plugin_test_targets} ${out_targets})
  
  # Debug config runtime files required for the plugin test mule (Qt image format plugins)
  set(plugintest_debug_src_dir "${ARCH_PREBUILT_DIRS_DEBUG}/imageformats")
  set(plugintest_debug_files
    qgifd4.dll
    qicod4.dll
    qjpegd4.dll
    qmngd4.dll
    qsvgd4.dll
    qtiffd4.dll
    )
  copy_if_different(
    ${plugintest_debug_src_dir}
    "${CMAKE_CURRENT_BINARY_DIR}/Debug/imageformats"
    out_targets
    ${plugintest_debug_files}
    )
  set(plugin_test_targets ${plugin_test_targets} ${out_targets})

  # Debug config runtime files required for the plugin test mule (Qt codec plugins)
  set(plugintest_debug_src_dir "${ARCH_PREBUILT_DIRS_DEBUG}/codecs")
  set(plugintest_debug_files
    qcncodecsd4.dll
    qjpcodecsd4.dll
    qkrcodecsd4.dll
    qtwcodecsd4.dll
    )
  copy_if_different(
    ${plugintest_debug_src_dir}
    "${CMAKE_CURRENT_BINARY_DIR}/Debug/codecs"
    out_targets
    ${plugintest_debug_files}
    )
  set(plugin_test_targets ${plugin_test_targets} ${out_targets})
 
  # Release & ReleaseDebInfo config runtime files required for the plugin test mule
  set(plugintest_release_src_dir "${ARCH_PREBUILT_DIRS_RELEASE}")
  set(plugintest_release_files
    libeay32.dll
    libglib-2.0-0.dll
    libgmodule-2.0-0.dll
    libgobject-2.0-0.dll
    libgthread-2.0-0.dll
    qtcore4.dll
    qtgui4.dll
    qtnetwork4.dll
    qtopengl4.dll
    qtwebkit4.dll
    qtxmlpatterns4.dll
    ssleay32.dll
    )
  copy_if_different(
    ${plugintest_release_src_dir}
    "${CMAKE_CURRENT_BINARY_DIR}/Release"
    out_targets
    ${plugintest_release_files}
    )
  set(plugin_test_targets ${plugin_test_targets} ${out_targets})

  copy_if_different(
    ${plugintest_release_src_dir}
    "${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo"
    out_targets
    ${plugintest_release_files}
    )
  set(plugin_test_targets ${plugin_test_targets} ${out_targets})

  # Release & ReleaseDebInfo config runtime files required for the plugin test mule (Qt image format plugins)
  set(plugintest_release_src_dir "${ARCH_PREBUILT_DIRS_RELEASE}/imageformats")
  set(plugintest_release_files
    qgif4.dll
    qico4.dll
    qjpeg4.dll
    qmng4.dll
    qsvg4.dll
    qtiff4.dll
    )
  copy_if_different(
    ${plugintest_release_src_dir}
    "${CMAKE_CURRENT_BINARY_DIR}/Release/imageformats"
    out_targets
    ${plugintest_release_files}
    )
  set(plugin_test_targets ${plugin_test_targets} ${out_targets})

  copy_if_different(
    ${plugintest_release_src_dir}
    "${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo/imageformats"
    out_targets
    ${plugintest_release_files}
    )
  set(plugin_test_targets ${plugin_test_targets} ${out_targets})

  # Release & ReleaseDebInfo config runtime files required for the plugin test mule (Qt codec plugins)
  set(plugintest_release_src_dir "${ARCH_PREBUILT_DIRS_RELEASE}/codecs")
  set(plugintest_release_files
    qcncodecs4.dll  
    qjpcodecs4.dll  
    qkrcodecs4.dll  
    qtwcodecs4.dll  
    )
  copy_if_different(
    ${plugintest_release_src_dir}
    "${CMAKE_CURRENT_BINARY_DIR}/Release/codecs"
    out_targets
    ${plugintest_release_files}
    )
  set(plugin_test_targets ${plugin_test_targets} ${out_targets})

  copy_if_different(
    ${plugintest_release_src_dir}
    "${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo/codecs"
    out_targets
    ${plugintest_release_files}
    )
  set(plugin_test_targets ${plugin_test_targets} ${out_targets})
 
   add_custom_target(copy_plugintest_libs ALL
     DEPENDS 
     ${plugin_test_targets}
     )

  add_dependencies(llmediaplugintest copy_plugintest_libs)

endif(WINDOWS)

ll_deploy_sharedlibs_command(llmediaplugintest) 
