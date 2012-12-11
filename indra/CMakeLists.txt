# -*- cmake -*-

# cmake_minimum_required should appear before any
# other commands to guarantee full compatibility
# with the version specified

# The "cmake -E touch" command was released with 2.4.8.
cmake_minimum_required(VERSION 2.4.8 FATAL_ERROR)

# This makes cmake 2.6 not complain about version 2.4 compatibility.
if (COMMAND cmake_policy)
  cmake_policy(SET CMP0003 OLD)
endif (COMMAND cmake_policy)

set(ROOT_PROJECT_NAME "SecondLife" CACHE STRING
    "The root project/makefile/solution name. Defaults to SecondLife.")
project(${ROOT_PROJECT_NAME})

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(Variables)

if (DARWIN)
  # 2.6.4 fixes a Mac bug in get_target_property(... "SLPlugin" LOCATION):
  # before that version it returns "pathname/SLPlugin", whereas the correct
  # answer is "pathname/SLPlugin.app/Contents/MacOS/SLPlugin".
  cmake_minimum_required(VERSION 2.6.4 FATAL_ERROR)
endif (DARWIN)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Build type.  One of: Debug Release RelWithDebInfo" FORCE)
endif (NOT CMAKE_BUILD_TYPE)

# For the library installation process;
# see cmake/Prebuild.cmake for the counterpart code.
if ("${CMAKE_SOURCE_DIR}/../autobuild.xml" IS_NEWER_THAN "${CMAKE_BINARY_DIR}/temp/sentinel_installed")
  file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/temp)
  file(WRITE ${CMAKE_BINARY_DIR}/temp/sentinel_installed "0")
endif ("${CMAKE_SOURCE_DIR}/../autobuild.xml" IS_NEWER_THAN "${CMAKE_BINARY_DIR}/temp/sentinel_installed")
add_subdirectory(cmake)

add_subdirectory(${LIBS_OPEN_PREFIX}llaudio)
add_subdirectory(${LIBS_OPEN_PREFIX}llappearance)
add_subdirectory(${LIBS_OPEN_PREFIX}llcharacter)
add_subdirectory(${LIBS_OPEN_PREFIX}llcommon)
add_subdirectory(${LIBS_OPEN_PREFIX}llcorehttp)
add_subdirectory(${LIBS_OPEN_PREFIX}llimage)
add_subdirectory(${LIBS_OPEN_PREFIX}llkdu)
add_subdirectory(${LIBS_OPEN_PREFIX}llimagej2coj)
add_subdirectory(${LIBS_OPEN_PREFIX}llinventory)
add_subdirectory(${LIBS_OPEN_PREFIX}llmath)
add_subdirectory(${LIBS_OPEN_PREFIX}llmessage)
add_subdirectory(${LIBS_OPEN_PREFIX}llprimitive)
add_subdirectory(${LIBS_OPEN_PREFIX}llrender)
add_subdirectory(${LIBS_OPEN_PREFIX}llvfs)
add_subdirectory(${LIBS_OPEN_PREFIX}llwindow)
add_subdirectory(${LIBS_OPEN_PREFIX}llxml)

add_subdirectory(${LIBS_OPEN_PREFIX}lscript)

if (WINDOWS AND EXISTS ${LIBS_CLOSED_DIR}copy_win_scripts)
  add_subdirectory(${LIBS_CLOSED_PREFIX}copy_win_scripts)
endif (WINDOWS AND EXISTS ${LIBS_CLOSED_DIR}copy_win_scripts)

add_custom_target(viewer)
add_subdirectory(${LIBS_OPEN_PREFIX}llcrashlogger)
add_subdirectory(${LIBS_OPEN_PREFIX}llplugin)
add_subdirectory(${LIBS_OPEN_PREFIX}llui)
add_subdirectory(${LIBS_OPEN_PREFIX}viewer_components)

# Legacy C++ tests. Build always, run if LL_TESTS is true.
add_subdirectory(${VIEWER_PREFIX}test)

# viewer media plugins
add_subdirectory(${LIBS_OPEN_PREFIX}media_plugins)

# llplugin testbed code (is this the right way to include it?)
if (LL_TESTS AND NOT LINUX)
  add_subdirectory(${VIEWER_PREFIX}test_apps/llplugintest)
endif (LL_TESTS AND NOT LINUX)

if (LINUX)
  add_subdirectory(${VIEWER_PREFIX}linux_crash_logger)
  add_subdirectory(${VIEWER_PREFIX}linux_updater)
  if (INSTALL_PROPRIETARY)
      include(LLAppearanceUtility)
      add_subdirectory(${LLAPPEARANCEUTILITY_SRC_DIR} ${LLAPPEARANCEUTILITY_BIN_DIR})
  endif (INSTALL_PROPRIETARY)
  add_dependencies(viewer linux-crash-logger-strip-target linux-updater)
elseif (DARWIN)
  add_subdirectory(${VIEWER_PREFIX}mac_crash_logger)
  add_subdirectory(${VIEWER_PREFIX}mac_updater)
  add_dependencies(viewer mac-updater mac-crash-logger)
elseif (WINDOWS)
  add_subdirectory(${VIEWER_PREFIX}win_crash_logger)
  # cmake EXISTS requires an absolute path, see indra/cmake/Variables.cmake
  if (EXISTS ${VIEWER_DIR}win_setup)
    add_subdirectory(${VIEWER_DIR}win_setup)
  endif (EXISTS ${VIEWER_DIR}win_setup)
  add_subdirectory(${VIEWER_PREFIX}win_updater)
  # add_dependencies(viewer windows-updater windows-setup windows-crash-logger)
  add_dependencies(viewer windows-updater windows-crash-logger)
elseif (SOLARIS)
  add_subdirectory(solaris_crash_logger)
  add_dependencies(viewer solaris-crash-logger)
endif (LINUX)

add_subdirectory(${VIEWER_PREFIX}newview)
add_dependencies(viewer secondlife-bin)

if (LL_TESTS)
  # Define after the custom targets are created so
  # individual apps can add themselves as dependencies
  add_subdirectory(${INTEGRATION_TESTS_PREFIX}integration_tests)
endif (LL_TESTS)
