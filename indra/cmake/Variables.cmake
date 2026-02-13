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

# Location of scripts directory
set(SCRIPTS_DIR ${INDRA_SOURCE_DIR}/../scripts)

# Select arch based on requested target processor
string(TOLOWER ${CMAKE_SYSTEM_PROCESSOR} processor_lower)
if(processor_lower STREQUAL "arm64")
  set(ARCH arm64)
else()
  set(ARCH x86_64)
endif()

# Only 64-bit architectures are support
set(ADDRESS_SIZE 64)

# Determine build platform
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  set(WINDOWS ON BOOL FORCE)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(LINUX ON BOOl FORCE)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(DARWIN ON BOOL FORCE)
endif ()

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set(COMPILER_IS_MSVC ON BOOL FORCE)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  if (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    set(COMPILER_IS_CLANG_CL ON BOOL FORCE)
  else()
    set(COMPILER_IS_CLANG ON BOOL FORCE)
  endif()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(COMPILER_IS_GCC ON BOOL FORCE)
endif ()


# Check if generator is multiconfig
get_property(LL_GENERATOR_IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

# Compatability with legacy cmake flags
if(DEFINED LL_TESTS)
  set(BUILD_TESTING ${LL_TESTS} CACHE BOOL "Build and run unit and integration tests: disable for build timing runs to reduce variation" FORCE)
endif()
