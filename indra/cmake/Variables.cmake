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

include_guard()

if (NOT USE_VCPKG)
  # Switches set here and in 00-Common.cmake must agree with
  # https://bitbucket.org/lindenlab/viewer-build-variables/src/tip/variables
  # Reading $LL_BUILD is an attempt to directly use those switches.
  if ("$ENV{AUTOBUILD_ADDRSIZE}" STREQUAL "" AND "${AUTOBUILD_ADDRSIZE_ENV}" STREQUAL "" )
    message(FATAL_ERROR "Environment variable AUTOBUILD_ADDRSIZE must be set")
  elseif("$ENV{AUTOBUILD_ADDRSIZE}" STREQUAL "")
    set( ENV{AUTOBUILD_ADDRSIZE} "${AUTOBUILD_ADDRSIZE_ENV}" )
    message( "Setting ENV{AUTOBUILD_ADDRSIZE} to cached variable ${AUTOBUILD_ADDRSIZE_ENV}" )
  else()
    set( AUTOBUILD_ADDRSIZE_ENV "$ENV{AUTOBUILD_ADDRSIZE}" CACHE STRING "Save environment AUTOBUILD_ADDRSIZE" FORCE )
  endif ()

  if ("$ENV{AUTOBUILD_PLATFORM}" STREQUAL "" AND "${AUTOBUILD_PLATFORM_ENV}" STREQUAL "" )
    message(FATAL_ERROR "Environment variable AUTOBUILD_PLATFORM must be set")
  elseif("$ENV{AUTOBUILD_PLATFORM}" STREQUAL "")
    set( ENV{AUTOBUILD_PLATFORM} "${AUTOBUILD_PLATFORM_ENV}" )
    message( "Setting ENV{AUTOBUILD_PLATFORM} to cached variable ${AUTOBUILD_PLATFORM_ENV}" )
  else()
    set( AUTOBUILD_PLATFORM_ENV "$ENV{AUTOBUILD_PLATFORM}" CACHE STRING "Save environment AUTOBUILD_PLATFORM" FORCE )
  endif ()
endif()

set(SCRIPTS_PREFIX ../scripts)
set(SCRIPTS_DIR ${CMAKE_SOURCE_DIR}/${SCRIPTS_PREFIX})
set(VIEWER_DIR ${CMAKE_SOURCE_DIR}/${VIEWER_PREFIX})

set(AUTOBUILD_INSTALL_DIR ${CMAKE_BINARY_DIR}/packages)

set(LIBS_PREBUILT_DIR ${AUTOBUILD_INSTALL_DIR} CACHE PATH
    "Location of prebuilt libraries.")

set(TEMPLATE_VERIFIER_OPTIONS "" CACHE STRING "Options for scripts/template_verifier.py")
set(TEMPLATE_VERIFIER_MASTER_URL "https://github.com/secondlife/master-message-template/raw/master/message_template.msg" CACHE STRING "Location of the master message template")

# We only support 64bit architectures currently
set(ARCH x86_64)
set(ADDRESS_SIZE 64)

# Determine build platform
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  set(WINDOWS ON BOOL FORCE)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(LINUX ON BOOl FORCE)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(DARWIN ON BOOL FORCE)
endif ()

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  # Only turn on headless if we can find osmesa libraries.
  find_package(PkgConfig)
  pkg_check_modules(OSMESA IMPORTED_TARGET GLOBAL osmesa)
  if (OSMESA_FOUND)
   set(BUILD_HEADLESS ON CACHE BOOL "Build headless libraries.")
  endif (OSMESA_FOUND)
endif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")

# Default deploy grid
set(GRID agni CACHE STRING "Target Grid")

set(ENABLE_SIGNING OFF CACHE BOOL "Enable signing the viewer")
set(SIGNING_IDENTITY "" CACHE STRING "Specifies the signing identity to use, if necessary.")

set(VERSION_BUILD "0" CACHE STRING "Revision number passed in from the outside")

source_group("CMake Rules" FILES CMakeLists.txt)

get_property(LL_GENERATOR_IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

