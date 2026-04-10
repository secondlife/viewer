# -*- cmake -*-
#
# Definitions of variables used throughout the Second Life build
# process.
#
# Platform variables:
#
#   DARWIN  - macOS
#   LINUX   - Linux
#   WINDOWS - Windows

# Switches set here and in 00-Common.cmake must agree with
# https://bitbucket.org/lindenlab/viewer-build-variables/src/tip/variables
# Reading $LL_BUILD is an attempt to directly use those switches.
if ("$ENV{LL_BUILD}" STREQUAL "" AND "${LL_BUILD_ENV}" STREQUAL "" )
  message(FATAL_ERROR "Environment variable LL_BUILD must be set")
elseif("$ENV{LL_BUILD}" STREQUAL "")
  set( ENV{LL_BUILD} "${LL_BUILD_ENV}" )
  message( "Setting ENV{LL_BUILD} to cached variable ${LL_BUILD_ENV}" )
else()
  set( LL_BUILD_ENV "$ENV{LL_BUILD}" CACHE STRING "Save environment" FORCE )
endif ()
include_guard()

# Relative and absolute paths to subtrees.

if(NOT DEFINED COMMON_CMAKE_DIR)
    set(COMMON_CMAKE_DIR "${CMAKE_SOURCE_DIR}/cmake")
endif(NOT DEFINED COMMON_CMAKE_DIR)

set(LIBS_CLOSED_PREFIX)
set(LIBS_OPEN_PREFIX)
set(SCRIPTS_PREFIX ../scripts)
set(VIEWER_PREFIX)
set(INTEGRATION_TESTS_PREFIX)
set(LL_TESTS ON CACHE BOOL "Build and run unit and integration tests (disable for build timing runs to reduce variation")
set(INCREMENTAL_LINK OFF CACHE BOOL "Use incremental linking on win32 builds (enable for faster links on some machines)")
set(ENABLE_MEDIA_PLUGINS ON CACHE BOOL "Turn off building media plugins if they are imported by third-party library mechanism")
set(VIEWER_SYMBOL_FILE "" CACHE STRING "Name of tarball into which to place symbol files")

if(LIBS_CLOSED_DIR)
  file(TO_CMAKE_PATH "${LIBS_CLOSED_DIR}" LIBS_CLOSED_DIR)
else(LIBS_CLOSED_DIR)
  set(LIBS_CLOSED_DIR ${CMAKE_SOURCE_DIR}/${LIBS_CLOSED_PREFIX})
endif(LIBS_CLOSED_DIR)
if(LIBS_COMMON_DIR)
  file(TO_CMAKE_PATH "${LIBS_COMMON_DIR}" LIBS_COMMON_DIR)
else(LIBS_COMMON_DIR)
  set(LIBS_COMMON_DIR ${CMAKE_SOURCE_DIR}/${LIBS_OPEN_PREFIX})
endif(LIBS_COMMON_DIR)
set(LIBS_OPEN_DIR ${LIBS_COMMON_DIR})

set(SCRIPTS_DIR ${CMAKE_SOURCE_DIR}/${SCRIPTS_PREFIX})
set(VIEWER_DIR ${CMAKE_SOURCE_DIR}/${VIEWER_PREFIX})

set(AUTOBUILD_INSTALL_DIR ${CMAKE_BINARY_DIR}/packages)

set(LIBS_PREBUILT_DIR ${AUTOBUILD_INSTALL_DIR} CACHE PATH
    "Location of prebuilt libraries.")

if (EXISTS ${CMAKE_SOURCE_DIR}/Server.cmake)
  # We use this as a marker that you can try to use the proprietary libraries.
  set(INSTALL_PROPRIETARY ON CACHE BOOL "Install proprietary binaries")
endif (EXISTS ${CMAKE_SOURCE_DIR}/Server.cmake)
set(TEMPLATE_VERIFIER_OPTIONS "" CACHE STRING "Options for scripts/template_verifier.py")
set(TEMPLATE_VERIFIER_MASTER_URL "https://github.com/secondlife/master-message-template/raw/master/message_template.msg" CACHE STRING "Location of the master message template")

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Build type.  One of: Debug Release RelWithDebInfo" FORCE)
endif (NOT CMAKE_BUILD_TYPE)

# If someone has specified an address size, use that to determine the
# architecture.  Otherwise, let the architecture specify the address size.
if (ADDRESS_SIZE EQUAL 32)
  set(ARCH i686)
elseif (ADDRESS_SIZE EQUAL 64)
  set(ARCH x86_64)
else (ADDRESS_SIZE EQUAL 32)
  # Use Python's platform.machine() since uname -m isn't available everywhere.
  execute_process(COMMAND
          "${PYTHON_EXECUTABLE}" "-c"
          "import platform; print( platform.machine() )"
          OUTPUT_VARIABLE ARCH OUTPUT_STRIP_TRAILING_WHITESPACE)
  string( REGEX MATCH ".*(64)$" RE_MATCH "${ARCH}" )
  if( RE_MATCH AND ${CMAKE_MATCH_1} STREQUAL "64" )
    set(ADDRESS_SIZE 64)
    set(ARCH x86_64)
  else()
    set(ADDRESS_SIZE 32)
    set(ARCH i686)
  endif()
endif (ADDRESS_SIZE EQUAL 32)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  set(WINDOWS ON BOOL FORCE)
endif (${CMAKE_SYSTEM_NAME} MATCHES "Windows")

# Default deploy grid
set(GRID agni CACHE STRING "Target Grid")

set(VIEWER_CHANNEL "Second Life Test" CACHE STRING "Viewer Channel Name")

set(ENABLE_SIGNING OFF CACHE BOOL "Enable signing the viewer")
set(SIGNING_IDENTITY "" CACHE STRING "Specifies the signing identity to use, if necessary.")

set(VERSION_BUILD "0" CACHE STRING "Revision number passed in from the outside")

set(USE_PRECOMPILED_HEADERS ON CACHE BOOL "Enable use of precompiled header directives where supported.")

source_group("CMake Rules" FILES CMakeLists.txt)

get_property(LL_GENERATOR_IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

