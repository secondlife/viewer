# -*- cmake -*-
project(llfbconnecttest)

include(00-Common)
include(FindOpenGL)
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

if (DARWIN)
    include(CMakeFindFrameworks)
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
endif (DARWIN)

### llfbconnecttest

set(llfbconnecttest_SOURCE_FILES
    llfbconnecttest.cpp
    llfbconnecttest.h
    bookmarks.txt
    )

add_executable(llfbconnecttest
    WIN32
    MACOSX_BUNDLE
    ${llfbconnecttest_SOURCE_FILES}
)

set_target_properties(llfbconnecttest
    PROPERTIES
    WIN32_EXECUTABLE
    FALSE
)

target_link_libraries(llfbconnecttest
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
  target_link_libraries(llfbconnecttest
    ${COREFOUNDATION_LIBRARY}
  )
endif (DARWIN)

add_dependencies(llfbconnecttest
  stage_third_party_libs
  SLPlugin
  media_plugin_webkit
  ${LLPLUGIN_LIBRARIES}
  ${LLMESSAGE_LIBRARIES}
  ${LLCOMMON_LIBRARIES}
)

# turn off weird GLUI pragma 
add_definitions(-DGLUI_NO_LIB_PRAGMA)

if (DARWIN OR LINUX)
  # glui.h contains code that triggers the "overloaded-virtual" warning in gcc.  
  set_source_files_properties(llfbconnecttest.cpp PROPERTIES COMPILE_FLAGS "-Wno-overloaded-virtual")
endif (DARWIN OR LINUX)

# Gather build products of the various dependencies into the build directory for the testbed.

if (DARWIN)
  # path inside the app bundle where we'll need to copy plugins and other related files
  set(PLUGINS_DESTINATION_DIR
    ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/llfbconnecttest.app/Contents/Resources
  )
  
  # create the Contents/Resources directory
  add_custom_command(
    TARGET llfbconnecttest POST_BUILD
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

set(BUILT_SLPLUGIN $<TARGET_FILE:SLPlugin>)
add_custom_command(TARGET llfbconnecttest POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${BUILT_SLPLUGIN}  ${PLUGINS_DESTINATION_DIR}
  DEPENDS ${BUILT_SLPLUGIN}
)

set(BUILT_LLCOMMON $<TARGET_FILE:llcommon>)
add_custom_command(TARGET llfbconnecttest POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${BUILT_LLCOMMON}  ${PLUGINS_DESTINATION_DIR}
  DEPENDS ${BUILT_LLCOMMON}
)


set(BUILT_WEBKIT_PLUGIN $<TARGET_FILE:media_plugin_webkit>)
add_custom_command(TARGET llfbconnecttest POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${BUILT_WEBKIT_PLUGIN}  ${PLUGINS_DESTINATION_DIR}
  DEPENDS ${BUILT_WEBKIT_PLUGIN}
)

# copy over bookmarks file if llfbconnecttest gets built
set(BUILT_LLFBCONNECTTEST $<TARGET_FILE:llfbconnecttest>)
add_custom_command(TARGET llfbconnecttest POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/bookmarks.txt ${CMAKE_CURRENT_BINARY_DIR}/
  DEPENDS ${BUILT_LLFBCONNECTTEST}
)

# also copy it to the same place as SLPlugin, which is what the mac wants...
add_custom_command(TARGET llfbconnecttest POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/bookmarks.txt ${PLUGINS_DESTINATION_DIR}
  DEPENDS ${BUILT_LLFBCONNECTTEST}
)

if(WINDOWS)
  #********************
  # Plugin test library deploy
  #
  # Debug config runtime files required for the FB connect test
  set(fbconnecttest_debug_src_dir "${ARCH_PREBUILT_DIRS_DEBUG}")
  set(fbconnecttest_debug_files
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
    ${fbconnecttest_debug_src_dir}
    "${CMAKE_CURRENT_BINARY_DIR}/Debug"
    out_targets
    ${fbconnecttest_debug_files}
    )
  set(fbconnect_test_targets ${fbconnect_test_targets} ${out_targets})
  
  # Debug config runtime files required for the FB connect test (Qt image format plugins)
  set(fbconecttest_debug_src_dir "${ARCH_PREBUILT_DIRS_DEBUG}/imageformats")
  set(fbconecttest_debug_files
    qgifd4.dll
    qicod4.dll
    qjpegd4.dll
    qmngd4.dll
    qsvgd4.dll
    qtiffd4.dll
    )
  copy_if_different(
    ${fbconecttest_debug_src_dir}
    "${CMAKE_CURRENT_BINARY_DIR}/Debug/imageformats"
    out_targets
    ${fbconecttest_debug_files}
    )
  set(fbconnect_test_targets ${fbconnect_test_targets} ${out_targets})

  # Debug config runtime files required for the FB connect test (Qt codec plugins)
  set(fbconnecttest_debug_src_dir "${ARCH_PREBUILT_DIRS_DEBUG}/codecs")
  set(fbconnecttest_debug_files
    qcncodecsd4.dll
    qjpcodecsd4.dll
    qkrcodecsd4.dll
    qtwcodecsd4.dll
    )
  copy_if_different(
    ${fbconnecttest_debug_src_dir}
    "${CMAKE_CURRENT_BINARY_DIR}/Debug/codecs"
    out_targets
    ${fbconnecttest_debug_files}
    )
  set(fbconnect_test_targets ${fbconnect_test_targets} ${out_targets})
 
  # Release & ReleaseDebInfo config runtime files required for the FB connect test
  set(fbconnecttest_release_src_dir "${ARCH_PREBUILT_DIRS_RELEASE}")
  set(fbconnecttest_release_files
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
    ${fbconnecttest_release_src_dir}
    "${CMAKE_CURRENT_BINARY_DIR}/Release"
    out_targets
    ${fbconnecttest_release_files}
    )
  set(fbconnect_test_targets ${fbconnect_test_targets} ${out_targets})

  copy_if_different(
    ${fbconnecttest_release_src_dir}
    "${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo"
    out_targets
    ${fbconnecttest_release_files}
    )
  set(fbconnect_test_targets ${fbconnect_test_targets} ${out_targets})

  # Release & ReleaseDebInfo config runtime files required for the FB connect test (Qt image format plugins)
  set(fbconnecttest_release_src_dir "${ARCH_PREBUILT_DIRS_RELEASE}/imageformats")
  set(fbconnecttest_release_files
    qgif4.dll
    qico4.dll
    qjpeg4.dll
    qmng4.dll
    qsvg4.dll
    qtiff4.dll
    )
  copy_if_different(
    ${fbconnecttest_release_src_dir}
    "${CMAKE_CURRENT_BINARY_DIR}/Release/imageformats"
    out_targets
    ${fbconnecttest_release_files}
    )
  set(fbconnect_test_targets ${fbconnect_test_targets} ${out_targets})

  copy_if_different(
    ${fbconnecttest_release_src_dir}
    "${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo/imageformats"
    out_targets
    ${fbconnecttest_release_files}
    )
  set(fbconnect_test_targets ${fbconnect_test_targets} ${out_targets})

  # Release & ReleaseDebInfo config runtime files required for the FB connect test (Qt codec plugins)
  set(fbconnecttest_release_src_dir "${ARCH_PREBUILT_DIRS_RELEASE}/codecs")
  set(fbconnecttest_release_files
    qcncodecs4.dll  
    qjpcodecs4.dll  
    qkrcodecs4.dll  
    qtwcodecs4.dll  
    )
  copy_if_different(
    ${fbconnecttest_release_src_dir}
    "${CMAKE_CURRENT_BINARY_DIR}/Release/codecs"
    out_targets
    ${fbconnecttest_release_files}
    )
  set(fbconnect_test_targets ${fbconnect_test_targets} ${out_targets})

  copy_if_different(
    ${fbconnecttest_release_src_dir}
    "${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo/codecs"
    out_targets
    ${fbconnecttest_release_files}
    )
  set(fbconnect_test_targets ${fbconnect_test_targets} ${out_targets})
 
   add_custom_target(copy_fbconnecttest_libs ALL
     DEPENDS 
     ${fbconnect_test_targets}
     )

  add_dependencies(llfbconnecttest copy_fbconnecttest_libs)

endif(WINDOWS)

if (DARWIN)
  set(fbconnecttest_release_src_dir "${ARCH_PREBUILT_DIRS_RELEASE}")
  set(fbconnecttest_release_files
    libexception_handler.dylib
    libaprutil-1.0.dylib
    libapr-1.0.dylib
    libexpat.1.5.2.dylib
    libQtCore.4.7.1.dylib
    libQtCore.4.dylib
    libQtGui.4.7.1.dylib
    libQtGui.4.dylib
    libQtNetwork.4.7.1.dylib
    libQtNetwork.4.dylib
    libQtOpenGL.4.7.1.dylib
    libQtOpenGL.4.dylib
    libQtWebKit.4.7.1.dylib
    libQtWebKit.4.dylib
    libQtSvg.4.7.1.dylib
    libQtSvg.4.dylib
    libQtXml.4.7.1.dylib
    libQtXml.4.dylib
    )
  copy_if_different(
    ${fbconnecttest_release_src_dir}
    "${PLUGINS_DESTINATION_DIR}"
    out_targets
    ${fbconnecttest_release_files}
    )
  set(fbconnect_test_targets ${fbconnect_test_targets} ${out_targets})

  # Release & ReleaseDebInfo config runtime files required for the FB connect test (Qt image format plugins)
  set(fbconnecttest_release_src_dir "${ARCH_PREBUILT_DIRS_PLUGINS}/imageformats")
  set(fbconnecttest_release_files
    libqgif.dylib
    libqico.dylib
    libqjpeg.dylib
    libqmng.dylib
    libqsvg.dylib
    libqtiff.dylib
    )
  copy_if_different(
    ${fbconnecttest_release_src_dir}
    "${PLUGINS_DESTINATION_DIR}/imageformats"
    out_targets
    ${fbconnecttest_release_files}
    )
  set(fbconnect_test_targets ${fbconnect_test_targets} ${out_targets})

  # Release & ReleaseDebInfo config runtime files required for the FB connect test (Qt codec plugins)
  set(fbconnecttest_release_src_dir "${ARCH_PREBUILT_DIRS_PLUGINS}/codecs")
  set(fbconnecttest_release_files
    libqcncodecs.dylib
    libqjpcodecs.dylib
    libqkrcodecs.dylib
    libqtwcodecs.dylib
    )
  copy_if_different(
    ${fbconnecttest_release_src_dir}
    "${PLUGINS_DESTINATION_DIR}/codecs"
    out_targets
    ${fbconnecttest_release_files}
    )
  set(fbconnect_test_targets ${fbconnect_test_targets} ${out_targets})

  add_custom_target(copy_fbconnecttest_libs ALL
    DEPENDS 
    ${fbconnect_test_targets}
    )

  add_dependencies(llfbconnecttest copy_fbconnecttest_libs)
endif (DARWIN)

if (LINUX)

endif (LINUX)

ll_deploy_sharedlibs_command(llfbconnecttest) 
