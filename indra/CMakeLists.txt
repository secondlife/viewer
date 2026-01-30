# -*- cmake -*-

# cmake_minimum_required should appear before any
# other commands to guarantee full compatibility
# with the version specified
# 3.16 is our minimum required cmake version while 4.2 is our cmake policy max version(meaning any policies of version 4.2 or below are set to NEW)
cmake_minimum_required(VERSION 3.26.0...4.2 FATAL_ERROR)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake"  "${CMAKE_CURRENT_BINARY_DIR}")

# Set our base channel
set(VIEWER_CHANNEL "Second Life Test" CACHE STRING "Viewer Channel Name")

# Build feature options
option(OS_DRAG_DROP "Build the viewer with OS level drag and drop turned on or off" ON)
option(USE_DISCORD "Enable Discord SDK and Rich Prescence support" OFF)
option(USE_KDU "Enable support for the Kakadu JPEG2000 decoding library" OFF)
option(USE_NDOF "Use NDOF space navigator joystick library." ON)
option(USE_LTO "Enable Link Time Optimization" OFF)
option(USE_NVAPI "Use NVidia NVAPI library for gpu profile support" ON)
option(USE_OPENAL "Enable support for the OpenAL audio engine" ON)
option(USE_OPENXR "Enable building with OpenXR support(Experimental)" OFF)
option(USE_PRECOMPILED_HEADERS "Enable use of precompiled header directives where supported." ON)
option(USE_SDL_WINDOW "Build with SDL based window management and input" OFF)
option(USE_TRACY "Use Tracy profiler." OFF)
option(USE_TRACY_ON_DEMAND "Use on-demand Tracy profiling." ON)
option(USE_TRACY_LOCAL_ONLY "Disallow remote Tracy profiling." OFF)
option(USE_TRACY_GPU "Use Tracy GPU profiling" OFF)
option(USE_TRACY_GUI "Enable building Tracy profiler GUI" OFF)
option(USE_VCPKG "Build with VCPKG libraries" ON)

# Configure crash reporting
option(RELEASE_CRASH_REPORTING "Enable use of crash reporting in release builds" OFF)
option(NON_RELEASE_CRASH_REPORTING "Enable use of crash reporting in developer builds" OFF)
set(VIEWER_SYMBOL_FILE "" CACHE STRING "Name of tarball into which to place symbol files")

# Tests
option(LL_TESTS "Build and run unit and integration tests (disable for build timing runs to reduce variation" OFF)

# Plugins
option(ENABLE_MEDIA_PLUGINS "Turn off building media plugins if they are imported by third-party library mechanism" ON)
option(EXAMPLEPLUGIN "EXAMPLEPLUGIN support for the llplugin/llmedia test apps." ON)
option(LIBVLCPLUGIN "LIBVLCPLUGIN support for the llplugin/llmedia test apps." ON)

# Sanitizers (Linux Only)
option(ENABLE_ASAN "Enable Address Sanitizer" OFF)
option(ENABLE_UBSAN "Enable Undefined Behavior Sanitizer" OFF)
option(ENABLE_THREADSAN "Enable Thread Sanitizer" OFF)

##########################################################################
# vcpkg feature flags MUST be set before the first project() call below  #
##########################################################################

# Setup proprietary libraries
if (INSTALL_PROPRIETARY)
  set(USE_DISCORD ON CACHE BOOL "Enable Discord SDK" FORCE)
  set(USE_KDU ON CACHE BOOL "Use Kakadu library." FORCE)
endif (INSTALL_PROPRIETARY)

# Setup SDL feature flag
if (USE_SDL_WINDOW)
  list(APPEND VCPKG_MANIFEST_FEATURES "sdl3")
endif()

# Default Tracy profiling on for test builds, but off for all others
string(TOLOWER ${VIEWER_CHANNEL} channel_lower)
if(channel_lower MATCHES "^second life test")
  set(USE_TRACY ON CACHE BOOL "Use Tracy profiler." FORCE)
endif()

# Setup Tracy feature flag
if (USE_TRACY)
  list(APPEND VCPKG_MANIFEST_FEATURES "tracy")
endif()

if (USE_TRACY_LOCAL_ONLY)
  list(APPEND VCPKG_MANIFEST_FEATURES "tracy-local")
endif()

if(USE_TRACY_ON_DEMAND)
  list(APPEND VCPKG_MANIFEST_FEATURES "tracy-on-demand")
endif()

if(USE_TRACY_GUI)
  list(APPEND VCPKG_MANIFEST_FEATURES "tracy-gui")
endif()

include(BootstrapVcpkg)

# Now create our default project after setting vcpkg flags
set(ROOT_PROJECT_NAME "SecondLife" CACHE STRING
    "The root project/makefile/solution name. Defaults to SecondLife.")
project(${ROOT_PROJECT_NAME})

# Set up Python
include(Python)

# Set up our base build system variable and compiler flags
include(Variables)
include(BuildVersion)
include(00-Common)
include(Linking)

# Don't bother with a MinSizeRel build.
set(CMAKE_CONFIGURATION_TYPES "Debug;RelWithDebInfo;Release" CACHE STRING "Supported build types." FORCE)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Build type.  One of: Debug Release RelWithDebInfo" FORCE)
endif (NOT CMAKE_BUILD_TYPE)

if (LL_TESTS)
  set(CMAKE_SKIP_TEST_ALL_DEPENDENCY FALSE)
  include(ProcessorCount)
  ProcessorCount(NUM_CORES)

  # Set parallel level to decrease test runtime
  set(CMAKE_CTEST_ARGUMENTS -j${NUM_CORES} --output-on-failure)

  # Set up our test targets
  set(CTEST_TEST_TARGET_ALIAS BUILD_AND_RUN_TESTS)
  include(CTest)

  set_target_properties(BUILD_AND_RUN_TESTS
        PROPERTIES
        FOLDER "CMakePredefinedTargets"
        )

  add_custom_target(BUILD_TESTS)
  add_dependencies(BUILD_AND_RUN_TESTS BUILD_TESTS)

  set_target_properties(BUILD_TESTS
        PROPERTIES
        FOLDER "CMakePredefinedTargets"
        )
endif ()

# Declare All Dependecy Includes
include(APR)
include(Boost)
include(bugsplat)
include(Collada-Dom)
include(CURL)
include(EXPAT)
include(FreeType)
include(GLEXT)
include(GLIB)
include(GLM)
include(Hunspell)
include(JPEG)
include(Mikktspace)
include(Meshoptimizer)
include(NDOF)
include(NanoSVG)
include(NVAPI)
include(OpenAL)
include(OpenGL)
include(OpenJPEG)
include(OpenSSL)
include(OpenXR)
include(PNG)
include(SDL3)
include(SSE2NEON)
include(TinyEXR)
include(Tracy)
include(Tut)
include(VHACD)
include(ViewerMiscLibs)
include(Vorbis)
include(ZLIBNG)

# Set up UI and Window linking
include(UI)

# Add our test macros
if (LL_TESTS)
  include(LLAddBuildTest)
endif ()

# Third party system library copy step
include(Copy3rdPartyLibs)

# Add our cmake modules to cmake
add_subdirectory(cmake)

if (USE_PRECOMPILED_HEADERS)
  add_subdirectory(llprecompiled)
endif ()
add_subdirectory(llaudio)
add_subdirectory(llappearance)
add_subdirectory(llcharacter)
add_subdirectory(llcommon)
add_subdirectory(llcorehttp)
add_subdirectory(llimage)
add_subdirectory(llkdu)
add_subdirectory(llimagej2coj)
add_subdirectory(llinventory)
add_subdirectory(llmath)
add_subdirectory(llmeshoptimizer)
add_subdirectory(llmessage)
add_subdirectory(llphysicsextensionsos)
add_subdirectory(llprimitive)
add_subdirectory(llrender)
add_subdirectory(llfilesystem)
add_subdirectory(llwebrtc)
add_subdirectory(llwindow)
add_subdirectory(llxml)

add_custom_target(viewer)

add_subdirectory(llplugin)
add_subdirectory(llui)
add_subdirectory(viewer_components)

if( LL_TESTS )
  # Legacy C++ tests.
  add_subdirectory(test)
endif()

if (ENABLE_MEDIA_PLUGINS)
  # viewer media plugins
  add_subdirectory(media_plugins)
endif (ENABLE_MEDIA_PLUGINS)

if (LINUX)
  # Avatar Appearance Utility
  add_subdirectory(llappearanceutility)
endif()

if (USE_BUGSPLAT)
    if (BUGSPLAT_DB)
        message(STATUS "Building with BugSplat; database '${BUGSPLAT_DB}'")
    else (BUGSPLAT_DB)
        message(WARNING "Building with BugSplat, but no database name set (BUGSPLAT_DB)")
    endif (BUGSPLAT_DB)
else (USE_BUGSPLAT)
    message(STATUS "Not building with BugSplat")
endif (USE_BUGSPLAT)

# Viewer
add_subdirectory(newview)
add_dependencies(viewer secondlife-bin)

add_subdirectory(doxygen EXCLUDE_FROM_ALL)

# sets the 'startup project' for debugging from visual studio.
set_property(
        DIRECTORY
        PROPERTY VS_STARTUP_PROJECT ${VIEWER_BINARY_NAME}
        )

if (LL_TESTS)
  # Define after the custom targets are created so
  # individual apps can add themselves as dependencies
  add_subdirectory(integration_tests)
endif (LL_TESTS)

