# -*- cmake -*-

project(media_plugin_libvlc)

include(00-Common)
include(LLCommon)
include(LLImage)
include(LLPlugin)
include(LLMath)
include(LLRender)
include(LLWindow)
include(Linking)
include(PluginAPI)
include(MediaPluginBase)
include(OpenGL)

include(LibVLCPlugin)

include_directories(
    ${LLPLUGIN_INCLUDE_DIRS}
    ${MEDIA_PLUGIN_BASE_INCLUDE_DIRS}
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
    ${LLIMAGE_INCLUDE_DIRS}
    ${LLRENDER_INCLUDE_DIRS}
    ${LLWINDOW_INCLUDE_DIRS}
    ${VLC_INCLUDE_DIR}
)
include_directories(SYSTEM
    ${LLCOMMON_SYSTEM_INCLUDE_DIRS}
    )


### media_plugin_libvlc

if(NOT ADDRESS_SIZE EQUAL 32)
  if(WINDOWS)
    ##add_definitions(/FIXED:NO)
  else(WINDOWS) # not windows therefore gcc LINUX and DARWIN
    add_definitions(-fPIC)
  endif(WINDOWS)
endif(NOT ADDRESS_SIZE EQUAL 32)

set(media_plugin_libvlc_SOURCE_FILES
    media_plugin_libvlc.cpp
    )

add_library(media_plugin_libvlc
    SHARED
    ${media_plugin_libvlc_SOURCE_FILES}
)

target_link_libraries(media_plugin_libvlc
  ${LLPLUGIN_LIBRARIES}
  ${MEDIA_PLUGIN_BASE_LIBRARIES}
  ${LLCOMMON_LIBRARIES}
  ${VLC_PLUGIN_LIBRARIES}
  ${PLUGIN_API_WINDOWS_LIBRARIES}
)

add_dependencies(media_plugin_libvlc
  ${LLPLUGIN_LIBRARIES}
  ${MEDIA_PLUGIN_BASE_LIBRARIES}
##${LLCOMMON_LIBRARIES}
  # Just say 'llcommon' here. LLCOMMON_LIBRARIES is specified for use in
  # target_link_libraries: it includes several Boost libraries, which are
  # absolutely NOT dependencies in the sense intended here. Those Boost
  # library macros, in turn, specify 'debug' and 'optimized' and a different
  # library name for each, producing several wordy errors:
  # Policy CMP0046 is not set: Error on non-existent dependency in
  # add_dependencies.
  # Really, the only dependency we should mention from LLCOMMON_LIBRARIES is
  # llcommon itself.
  llcommon
)

if (WINDOWS)
  set_target_properties(
    media_plugin_libvlc
    PROPERTIES
    LINK_FLAGS "/MANIFEST:NO /SAFESEH:NO /NODEFAULTLIB:LIBCMT"
    )
endif (WINDOWS)

if (DARWIN)
  # Don't prepend 'lib' to the executable name, and don't embed a full path in the library's install name
  set_target_properties(
    media_plugin_libvlc
    PROPERTIES
    PREFIX ""
    BUILD_WITH_INSTALL_RPATH 1
    INSTALL_NAME_DIR "@executable_path"
    LINK_FLAGS "-exported_symbols_list ${CMAKE_CURRENT_SOURCE_DIR}/../base/media_plugin_base.exp"
  )

endif (DARWIN)
