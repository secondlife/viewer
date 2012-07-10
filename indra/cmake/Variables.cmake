# -*- cmake -*-
#
# Definitions of variables used throughout the Second Life build
# process.
#
# Platform variables:
#
#   DARWIN  - Mac OS X
#   LINUX   - Linux
#   WINDOWS - Windows
#
# What to build:
#
#   VIEWER - viewer and other viewer-side components
#   SERVER - simulator and other server-side bits


# Relative and absolute paths to subtrees.

if(NOT DEFINED COMMON_CMAKE_DIR)
    set(COMMON_CMAKE_DIR "${CMAKE_SOURCE_DIR}/cmake")
endif(NOT DEFINED COMMON_CMAKE_DIR)

set(LIBS_CLOSED_PREFIX)
set(LIBS_OPEN_PREFIX)
set(LIBS_SERVER_PREFIX)
set(SCRIPTS_PREFIX ../scripts)
set(SERVER_PREFIX)
set(VIEWER_PREFIX)
set(INTEGRATION_TESTS_PREFIX)
set(LL_TESTS ON CACHE BOOL "Build and run unit and integration tests (disable for build timing runs to reduce variation")
set(INCREMENTAL_LINK OFF CACHE BOOL "Use incremental linking on win32 builds (enable for faster links on some machines)")

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

set(LIBS_SERVER_DIR ${CMAKE_SOURCE_DIR}/${LIBS_SERVER_PREFIX})
set(SCRIPTS_DIR ${CMAKE_SOURCE_DIR}/${SCRIPTS_PREFIX})
set(SERVER_DIR ${CMAKE_SOURCE_DIR}/${SERVER_PREFIX})
set(VIEWER_DIR ${CMAKE_SOURCE_DIR}/${VIEWER_PREFIX})

set(AUTOBUILD_INSTALL_DIR ${CMAKE_BINARY_DIR}/packages)

set(LIBS_PREBUILT_DIR ${AUTOBUILD_INSTALL_DIR} CACHE PATH
    "Location of prebuilt libraries.")

if (EXISTS ${CMAKE_SOURCE_DIR}/Server.cmake)
  # We use this as a marker that you can try to use the proprietary libraries.
  set(INSTALL_PROPRIETARY ON CACHE BOOL "Install proprietary binaries")
endif (EXISTS ${CMAKE_SOURCE_DIR}/Server.cmake)
set(TEMPLATE_VERIFIER_OPTIONS "" CACHE STRING "Options for scripts/template_verifier.py")
set(TEMPLATE_VERIFIER_MASTER_URL "http://bitbucket.org/lindenlab/master-message-template/raw/tip/message_template.msg" CACHE STRING "Location of the master message template")

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Build type.  One of: Debug Release RelWithDebInfo" FORCE)
endif (NOT CMAKE_BUILD_TYPE)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  set(WINDOWS ON BOOL FORCE)
  set(ARCH i686)
  set(LL_ARCH ${ARCH}_win32)
  set(LL_ARCH_DIR ${ARCH}-win32)
  set(WORD_SIZE 32)
endif (${CMAKE_SYSTEM_NAME} MATCHES "Windows")

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(LINUX ON BOOl FORCE)

  # If someone has specified a word size, use that to determine the
  # architecture.  Otherwise, let the architecture specify the word size.
  if (WORD_SIZE EQUAL 32)
    set(ARCH i686)
  elseif (WORD_SIZE EQUAL 64)
    set(ARCH x86_64)
  else (WORD_SIZE EQUAL 32)
    execute_process(COMMAND uname -m COMMAND sed s/i.86/i686/
                    OUTPUT_VARIABLE ARCH OUTPUT_STRIP_TRAILING_WHITESPACE)
    if (ARCH STREQUAL x86_64)
      set(WORD_SIZE 64)
    else (ARCH STREQUAL x86_64)
      set(WORD_SIZE 32)
    endif (ARCH STREQUAL x86_64)
  endif (WORD_SIZE EQUAL 32)

  set(LL_ARCH ${ARCH}_linux)
  set(LL_ARCH_DIR ${ARCH}-linux)
endif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(DARWIN 1)
  
  # To support a different SDK update these Xcode settings:
  set(CMAKE_OSX_DEPLOYMENT_TARGET 10.5)
  set(CMAKE_OSX_SYSROOT macosx10.6)
  set(CMAKE_XCODE_ATTRIBUTE_GCC_VERSION "com.apple.compilers.llvmgcc42")
  set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT dwarf-with-dsym)

  # NOTE: To attempt an i386/PPC Universal build, add this on the configure line:
  # -DCMAKE_OSX_ARCHITECTURES:STRING='i386;ppc'
  # Build only for i386 by default, system default on MacOSX 10.6 is x86_64
  if (NOT CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES i386)
  endif (NOT CMAKE_OSX_ARCHITECTURES)

  if (CMAKE_OSX_ARCHITECTURES MATCHES "i386" AND CMAKE_OSX_ARCHITECTURES MATCHES "ppc")
    set(ARCH universal)
  else (CMAKE_OSX_ARCHITECTURES MATCHES "i386" AND CMAKE_OSX_ARCHITECTURES MATCHES "ppc")
    if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "ppc")
      set(ARCH ppc)
    else (${CMAKE_SYSTEM_PROCESSOR} MATCHES "ppc")
      set(ARCH i386)
    endif (${CMAKE_SYSTEM_PROCESSOR} MATCHES "ppc")
  endif (CMAKE_OSX_ARCHITECTURES MATCHES "i386" AND CMAKE_OSX_ARCHITECTURES MATCHES "ppc")

  set(LL_ARCH ${ARCH}_darwin)
  set(LL_ARCH_DIR universal-darwin)
  set(WORD_SIZE 32)
endif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

# Default deploy grid
set(GRID agni CACHE STRING "Target Grid")

set(VIEWER ON CACHE BOOL "Build Second Life viewer.")
set(VIEWER_CHANNEL "LindenDeveloper" CACHE STRING "Viewer Channel Name")
set(VIEWER_LOGIN_CHANNEL ${VIEWER_CHANNEL} CACHE STRING "Fake login channel for A/B Testing")

set(VERSION_BUILD "0" CACHE STRING "Revision number passed in from the outside")
set(STANDALONE OFF CACHE BOOL "Do not use Linden-supplied prebuilt libraries.")
set(UNATTENDED OFF CACHE BOOL "Should be set to ON for building with VC Express editions.")

if (NOT STANDALONE AND EXISTS ${CMAKE_SOURCE_DIR}/llphysics)
    set(SERVER ON CACHE BOOL "Build Second Life server software.")
endif (NOT STANDALONE AND EXISTS ${CMAKE_SOURCE_DIR}/llphysics)

if (LINUX AND SERVER AND VIEWER)
  MESSAGE(FATAL_ERROR "
The indra source does not currently support building SERVER and VIEWER at the same time.
Please set one of these values to OFF in your CMake cache file.
(either by running ccmake or by editing CMakeCache.txt by hand)
For more information, please see JIRA DEV-14943 - Cmake Linux cannot build both VIEWER and SERVER in one build environment
  ")
endif (LINUX AND SERVER AND VIEWER)


set(USE_PRECOMPILED_HEADERS ON CACHE BOOL "Enable use of precompiled header directives where supported.")

source_group("CMake Rules" FILES CMakeLists.txt)

