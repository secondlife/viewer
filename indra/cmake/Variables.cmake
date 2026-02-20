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
if(processor_lower STREQUAL "arm64" OR processor_lower STREQUAL "aarch64")
  set(ARCH arm64)
  set(BUILD_TARGET_IS_ARM64 ON CACHE INTERNAL "ARM64 BUILD" FORCE)
else()
  set(ARCH x86_64)
  set(BUILD_TARGET_IS_X86_64 ON CACHE INTERNAL "X86_64 BUILD" FORCE)
endif()

# Only 64-bit architectures are supported
set(ADDRESS_SIZE 64)

# Determine build platform
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  set(WINDOWS ON CACHE INTERNAL "Windows Build" FORCE)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(LINUX ON CACHE INTERNAL "Linux Build" FORCE)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(DARWIN ON CACHE INTERNAL "Darwin Build" FORCE)
endif ()

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set(COMPILER_IS_MSVC ON CACHE INTERNAL "MSVC BUILD" FORCE)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  if (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    set(COMPILER_IS_CLANG_CL ON CACHE INTERNAL "CLANG-CL BUILD" FORCE)
  else()
    set(COMPILER_IS_CLANG ON CACHE INTERNAL "CLANG BUILD" FORCE)
  endif()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(COMPILER_IS_GCC ON CACHE INTERNAL "GCC BUILD" FORCE)
endif ()


# Check if generator is multiconfig
get_property(LL_GENERATOR_IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

# Compatibility with legacy cmake flags
if(DEFINED LL_TESTS)
  set(BUILD_TESTING ${LL_TESTS} CACHE BOOL "Build and run unit and integration tests: disable for build timing runs to reduce variation" FORCE)
endif()
