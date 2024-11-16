# -*- cmake -*-

# cmake_minimum_required should appear before any
# other commands to guarantee full compatibility
# with the version specified
## 3.8 added VS_DEBUGGER_WORKING_DIRECTORY support
## 3.13/12 is needed for add_link_options/add_compile_definitions
## 3.14 added FILE CREATE_LINK
## 3.16 is needed for target_precompile_headers
## Nicky: Ideally we want at least 3.21 for good preset support
##   We're not there yet, but once done, there is a kludge in Linking.cmake
#    "if(${CMAKE_VERSION} VERSION_LESS "3.20.0")" that can also be removed
cmake_minimum_required(VERSION 3.16.0 FATAL_ERROR)

set(ROOT_PROJECT_NAME "SecondLife" CACHE STRING
    "The root project/makefile/solution name. Defaults to SecondLife.")
project(${ROOT_PROJECT_NAME})

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake"  "${CMAKE_CURRENT_BINARY_DIR}")

include(conanbuildinfo OPTIONAL RESULT_VARIABLE USE_CONAN )
if( USE_CONAN )
    set( USE_CONAN ON )
    set( USE_AUTOBUILD_3P OFF )
    conan_basic_setup(TARGETS NO_OUTPUT_DIRS)
    add_compile_definitions(LL_USESYSTEMLIBS USE_CONAN NO_AUTOBUILD_3P)
else()
  set( USE_CONAN OFF )
  set( USE_AUTOBUILD_3P ON )
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(Variables)
include(BuildVersion)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Build type.  One of: Debug Release RelWithDebInfo" FORCE)
endif (NOT CMAKE_BUILD_TYPE)

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
add_subdirectory(${LIBS_OPEN_PREFIX}llmeshoptimizer)
add_subdirectory(${LIBS_OPEN_PREFIX}llmessage)
add_subdirectory(${LIBS_OPEN_PREFIX}llprimitive)
add_subdirectory(${LIBS_OPEN_PREFIX}llrender)
add_subdirectory(${LIBS_OPEN_PREFIX}llfilesystem)
add_subdirectory(${LIBS_OPEN_PREFIX}llwebrtc)
add_subdirectory(${LIBS_OPEN_PREFIX}llwindow)
add_subdirectory(${LIBS_OPEN_PREFIX}llxml)

if (WINDOWS AND EXISTS ${LIBS_CLOSED_DIR}copy_win_scripts)
  add_subdirectory(${LIBS_CLOSED_PREFIX}copy_win_scripts)
endif (WINDOWS AND EXISTS ${LIBS_CLOSED_DIR}copy_win_scripts)

add_custom_target(viewer)

if (NOT USE_BUGSPLAT)
add_subdirectory(${LIBS_OPEN_PREFIX}llcrashlogger)
endif (NOT USE_BUGSPLAT)

add_subdirectory(${LIBS_OPEN_PREFIX}llplugin)
add_subdirectory(${LIBS_OPEN_PREFIX}llui)
add_subdirectory(${LIBS_OPEN_PREFIX}viewer_components)

if( LL_TESTS )
# Legacy C++ tests. Build always, run if LL_TESTS is true.
add_subdirectory(${VIEWER_PREFIX}test)
endif()

if (ENABLE_MEDIA_PLUGINS)
# viewer media plugins
add_subdirectory(${LIBS_OPEN_PREFIX}media_plugins)
endif (ENABLE_MEDIA_PLUGINS)

# if (LINUX)
#   if (INSTALL_PROPRIETARY)
#       include(LLAppearanceUtility)
#       add_subdirectory(${LLAPPEARANCEUTILITY_SRC_DIR} ${LLAPPEARANCEUTILITY_BIN_DIR})
#   endif (INSTALL_PROPRIETARY)
# elseif (WINDOWS)
#   # cmake EXISTS requires an absolute path, see indra/cmake/Variables.cmake
#   if (EXISTS ${VIEWER_DIR}win_setup)
#     add_subdirectory(${VIEWER_DIR}win_setup)
#   endif (EXISTS ${VIEWER_DIR}win_setup)
# endif (LINUX)

if (WINDOWS)
    # cmake EXISTS requires an absolute path, see indra/cmake/Variables.cmake
    if (EXISTS ${VIEWER_DIR}win_setup)
      add_subdirectory(${VIEWER_DIR}win_setup)
    endif (EXISTS ${VIEWER_DIR}win_setup)
endif (WINDOWS)

if (USE_BUGSPLAT)
    if (BUGSPLAT_DB)
        message(STATUS "Building with BugSplat; database '${BUGSPLAT_DB}'")
    else (BUGSPLAT_DB)
        message(WARNING "Building with BugSplat, but no database name set (BUGSPLAT_DB)")
    endif (BUGSPLAT_DB)
else (USE_BUGSPLAT)
    message(STATUS "Not building with BugSplat")
endif (USE_BUGSPLAT)

add_subdirectory(${VIEWER_PREFIX}newview)
add_dependencies(viewer secondlife-bin)

add_subdirectory(${VIEWER_PREFIX}doxygen EXCLUDE_FROM_ALL)

# sets the 'startup project' for debugging from visual studio.
set_property(
        DIRECTORY ${VIEWER_PREFIX}
        PROPERTY VS_STARTUP_PROJECT ${VIEWER_BINARY_NAME}
        )

if (LL_TESTS)
  # Define after the custom targets are created so
  # individual apps can add themselves as dependencies
  add_subdirectory(${INTEGRATION_TESTS_PREFIX}integration_tests)
endif (LL_TESTS)

