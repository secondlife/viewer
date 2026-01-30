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

set(SCRIPTS_DIR ${CMAKE_SOURCE_DIR}/../scripts)
set(VIEWER_DIR ${CMAKE_SOURCE_DIR})

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

  # Only turn on headless if we can find osmesa libraries.
  find_package(PkgConfig)
  pkg_check_modules(OSMESA IMPORTED_TARGET GLOBAL osmesa)
  if (OSMESA_FOUND)
   set(BUILD_HEADLESS ON CACHE BOOL "Build headless libraries.")
  endif (OSMESA_FOUND)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(DARWIN ON BOOL FORCE)
endif ()

# Default deploy grid
set(GRID agni CACHE STRING "Target Grid")

set(ENABLE_SIGNING OFF CACHE BOOL "Enable signing the viewer")
set(SIGNING_IDENTITY "" CACHE STRING "Specifies the signing identity to use, if necessary.")

set(VERSION_BUILD "0" CACHE STRING "Revision number passed in from the outside")

source_group("CMake Rules" FILES CMakeLists.txt)

get_property(LL_GENERATOR_IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

