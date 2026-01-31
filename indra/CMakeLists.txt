# -*- cmake -*-

# cmake_minimum_required should appear before any
# other commands to guarantee full compatibility
# with the version specified
# 3.26 is our minimum required cmake version while 4.2 is our cmake policy max version(meaning any policies of version 4.2 or below are set to NEW)
cmake_minimum_required(VERSION 3.26.0...4.2 FATAL_ERROR)

# Include our cmake module directory
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake"  "${CMAKE_CURRENT_BINARY_DIR}")

# Check if generator is multiconfig
get_property(LL_GENERATOR_IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

# Setup for options depending on other options
include(CMakeDependentOption)

# Set our base channel
set(VIEWER_CHANNEL "Second Life Test" CACHE STRING "Viewer Channel Name")

# Build feature options
option(OS_DRAG_DROP "Build the viewer with OS level drag and drop turned on or off" ON)
option(USE_NDOF "Use NDOF space navigator joystick library." ON)
option(USE_LTO "Enable Link Time Optimization" OFF)
option(USE_NVAPI "Use NVidia NVAPI library for gpu profile support" ON)
option(USE_OPENAL "Enable support for the OpenAL audio engine" ON)
option(USE_OPENXR "Enable building with OpenXR support(Experimental)" OFF)
option(USE_PRECOMPILED_HEADERS "Enable use of precompiled header directives where supported." ON)

# Proprietary Libraries
option(INSTALL_PROPRIETARY "Build with support for non-free libraries" OFF)
cmake_dependent_option(USE_KDU "Enable support for the Kakadu JPEG2000 decoding library" ON "INSTALL_PROPRIETARY" OFF)

# TODO: Replace with commented option below once Havok support is added to vcpkg
option(HAVOK "Use Havok physics library" OFF)
#cmake_dependent_option(HAVOK "Use Havok physics library" ON "INSTALL_PROPRIETARY" OFF)

# Uncomment when support for Discord is finalized
#cmake_dependent_option(USE_DISCORD "Enable Discord SDK and Rich Prescence support" ON "INSTALL_PROPRIETARY" OFF)

# SDL Options
cmake_dependent_option(USE_SDL_WINDOW "Build with SDL based window management and input" ON "UNIX;NOT APPLE" OFF)

# Configure crash reporting
option(RELEASE_CRASH_REPORTING "Enable use of crash reporting in release builds" OFF)
option(NON_RELEASE_CRASH_REPORTING "Enable use of crash reporting in developer builds" OFF)
set(VIEWER_SYMBOL_FILE "" CACHE STRING "Name of tarball into which to place symbol files")

# Bugsplat setup
if (INSTALL_PROPRIETARY AND (WIN32 OR APPLE))
    # Note that viewer_manifest.py makes decision based on BUGSPLAT_DB and not USE_BUGSPLAT
    if (BUGSPLAT_DB)
        set(USE_BUGSPLAT ON  CACHE BOOL "Use the BugSplat crash reporting system")
    else (BUGSPLAT_DB)
        set(USE_BUGSPLAT OFF CACHE BOOL "Use the BugSplat crash reporting system")
    endif (BUGSPLAT_DB)
else ()
    set(USE_BUGSPLAT OFF CACHE BOOL "Use the BugSplat crash reporting system")
    set(BUGSPLAT_DB "" CACHE STRING "BugSplat crash database name")
endif ()

# Tracy Profiler
# Default Tracy profiling on for test builds, but off for all others
set(BUILDING_TEST_CHANNEL OFF)
string(TOLOWER ${VIEWER_CHANNEL} channel_lower)
if(channel_lower MATCHES "^second life test")
  set(BUILDING_TEST_CHANNEL ON)
endif()
cmake_dependent_option(USE_TRACY "Enable support for Tracy profiler" ON "BUILDING_TEST_CHANNEL" OFF)
option(USE_TRACY_ON_DEMAND "Use on-demand Tracy profiling." ON)
option(USE_TRACY_LOCAL_ONLY "Disallow remote Tracy profiling." OFF)
option(USE_TRACY_GPU "Use Tracy GPU profiling" OFF)
option(USE_TRACY_GUI "Enable building Tracy profiler GUI" OFF)

# Tests
option(LL_TESTS "Build and run unit and integration tests: disable for build timing runs to reduce variation" OFF)

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

if (USE_KDU)
  list(APPEND VCPKG_MANIFEST_FEATURES "kdu")
endif()

# Setup SDL feature flag
if (USE_SDL_WINDOW)
  list(APPEND VCPKG_MANIFEST_FEATURES "sdl3")
endif()

# Setup Tracy feature flags
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

if(USE_OPENXR)
  list(APPEND VCPKG_MANIFEST_FEATURES "openxr")
endif()

if(USE_BUGSPLAT)
  list(APPEND VCPKG_MANIFEST_FEATURES "bugsplat")
endif()

# Export compile_commands.json for ninja and makefile generators
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Don't bother with a MinSizeRel build.
set(CMAKE_CONFIGURATION_TYPES "Debug;RelWithDebInfo;Release" CACHE STRING "Supported build types." FORCE)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Build type.  One of: Debug Release RelWithDebInfo" FORCE)
endif (NOT CMAKE_BUILD_TYPE)

# Now create our default project after setting vcpkg flags
set(ROOT_PROJECT_NAME "SecondLife" CACHE STRING
    "The root project/makefile/solution name. Defaults to SecondLife.")

# Setup VCPKG toolchains and targets
include(BootstrapVcpkg)

# Generate out base project after setting pre-generation variables
project(${ROOT_PROJECT_NAME})

# Set up Python
include(Python)

# Set up our base build system variable and compiler flags
include(Variables)
include(BuildVersion)
include(00-Common)
include(Linking)

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
include(Discord)
include(EXPAT)
include(FontConfig)
include(FreeType)
include(GLEXT)
include(GLIB)
include(GLM)
include(Havok)
include(Hunspell)
include(JPEG)
include(LLKDU)
include(LLPhysicsExtensions)
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
include(Vorbis)
include(WebRTC)
include(xxHash)
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

