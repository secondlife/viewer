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

# Switches set here and in 00-Common.cmake must agree with
# https://bitbucket.org/lindenlab/viewer-build-variables/src/tip/variables
# Reading $LL_BUILD is an attempt to directly use those switches.
if ("$ENV{LL_BUILD}" STREQUAL "")
  message(FATAL_ERROR "Environment variable LL_BUILD must be set")
endif ()

# Relative and absolute paths to subtrees.

if(NOT DEFINED ${CMAKE_CURRENT_LIST_FILE}_INCLUDED)
set(${CMAKE_CURRENT_LIST_FILE}_INCLUDED "YES")

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
set(TEMPLATE_VERIFIER_MASTER_URL "http://bitbucket.org/lindenlab/master-message-template/raw/tip/message_template.msg" CACHE STRING "Location of the master message template")

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Build type.  One of: Debug Release RelWithDebInfo" FORCE)
endif (NOT CMAKE_BUILD_TYPE)

# If someone has specified an address size, use that to determine the
# architecture.  Otherwise, let the architecture specify the address size.
if (ADDRESS_SIZE EQUAL 32)
  #message(STATUS "ADDRESS_SIZE is 32")
  set(ARCH i686)
elseif (ADDRESS_SIZE EQUAL 64)
  #message(STATUS "ADDRESS_SIZE is 64")
  set(ARCH x86_64)
else (ADDRESS_SIZE EQUAL 32)
  #message(STATUS "ADDRESS_SIZE is UNRECOGNIZED: '${ADDRESS_SIZE}'")
  # Use Python's platform.machine() since uname -m isn't available everywhere.
  # Even if you can assume cygwin uname -m, the answer depends on whether
  # you're running 32-bit cygwin or 64-bit cygwin! But even 32-bit Python will
  # report a 64-bit processor.
  execute_process(COMMAND
                  "${PYTHON_EXECUTABLE}" "-c"
                  "import platform; print platform.machine()"
                  OUTPUT_VARIABLE ARCH OUTPUT_STRIP_TRAILING_WHITESPACE)
  # We expect values of the form i386, i686, x86_64, AMD64.
  # In CMake, expressing ARCH.endswith('64') is awkward:
  string(LENGTH "${ARCH}" ARCH_LENGTH)
  math(EXPR ARCH_LEN_2 "${ARCH_LENGTH} - 2")
  string(SUBSTRING "${ARCH}" ${ARCH_LEN_2} 2 ARCH_LAST_2)
  if (ARCH_LAST_2 STREQUAL 64)
    #message(STATUS "ARCH is detected as 64; ARCH is ${ARCH}")
    set(ADDRESS_SIZE 64)
  else ()
    #message(STATUS "ARCH is detected as 32; ARCH is ${ARCH}")
    set(ADDRESS_SIZE 32)
  endif ()
endif (ADDRESS_SIZE EQUAL 32)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  set(WINDOWS ON BOOL FORCE)
  set(LL_ARCH ${ARCH}_win32)
  set(LL_ARCH_DIR ${ARCH}-win32)
endif (${CMAKE_SYSTEM_NAME} MATCHES "Windows")

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(LINUX ON BOOl FORCE)

  if (ADDRESS_SIZE EQUAL 32)
    set(DEB_ARCHITECTURE i386)
    set(FIND_LIBRARY_USE_LIB64_PATHS OFF)
    set(CMAKE_SYSTEM_LIBRARY_PATH /usr/lib32 ${CMAKE_SYSTEM_LIBRARY_PATH})
  else (ADDRESS_SIZE EQUAL 32)
    set(DEB_ARCHITECTURE amd64)
    set(FIND_LIBRARY_USE_LIB64_PATHS ON)
  endif (ADDRESS_SIZE EQUAL 32)

  execute_process(COMMAND dpkg-architecture -a${DEB_ARCHITECTURE} -qDEB_HOST_MULTIARCH 
      RESULT_VARIABLE DPKG_RESULT
      OUTPUT_VARIABLE DPKG_ARCH
      OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
  #message (STATUS "DPKG_RESULT ${DPKG_RESULT}, DPKG_ARCH ${DPKG_ARCH}")
  if (DPKG_RESULT EQUAL 0)
    set(CMAKE_LIBRARY_ARCHITECTURE ${DPKG_ARCH})
    set(CMAKE_SYSTEM_LIBRARY_PATH /usr/lib/${DPKG_ARCH} /usr/local/lib/${DPKG_ARCH} ${CMAKE_SYSTEM_LIBRARY_PATH})
  endif (DPKG_RESULT EQUAL 0)

  include(ConfigurePkgConfig)

  set(LL_ARCH ${ARCH}_linux)
  set(LL_ARCH_DIR ${ARCH}-linux)

  if (INSTALL_PROPRIETARY)
    # Only turn on headless if we can find osmesa libraries.
    include(FindPkgConfig)
    #pkg_check_modules(OSMESA osmesa)
    #if (OSMESA_FOUND)
    #  set(BUILD_HEADLESS ON CACHE BOOL "Build headless libraries.")
    #endif (OSMESA_FOUND)
  endif (INSTALL_PROPRIETARY)

endif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(DARWIN 1)

  string(REGEX MATCH "-mmacosx-version-min=([^ ]+)" scratch "$ENV{LL_BUILD}")
  set(CMAKE_OSX_DEPLOYMENT_TARGET "${CMAKE_MATCH_1}")
  message(STATUS "CMAKE_OSX_DEPLOYMENT_TARGET = '${CMAKE_OSX_DEPLOYMENT_TARGET}'")

  string(REGEX MATCH "-stdlib=([^ ]+)" scratch "$ENV{LL_BUILD}")
  set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "${CMAKE_MATCH_1}")
  message(STATUS "CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY = '${CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY}'")

  string(REGEX MATCH " -g([^ ]*)" scratch "$ENV{LL_BUILD}")
  set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "${CMAKE_MATCH_1}")
  # -gdwarf-2 is passed in LL_BUILD according to 00-COMPILE-LINK-RUN.txt.
  # However, when CMake 3.9.2 sees -gdwarf-2, it silently deletes the whole -g
  # switch, producing no symbols at all! The same thing happens if we specify
  # plain -g ourselves, i.e. CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT is
  # the empty string. Specifying -gdwarf-with-dsym or just -gdwarf drives a
  # different CMake behavior: it substitutes plain -g. As of 2017-09-19,
  # viewer-build-variables/variables still passes -gdwarf-2, which is the
  # no-symbols case. Set -gdwarf, triggering CMake to substitute plain -g --
  # at least that way we should get symbols, albeit mangled ones. It Would Be
  # Nice if CMake's behavior could be predicted from a consistent mental
  # model, instead of only observed experimentally.
  string(REPLACE "dwarf-2" "dwarf"
    CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT
    "${CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT}")
  message(STATUS "CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT = '${CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT}'")

  string(REGEX MATCH "-O([^ ]*)" scratch "$ENV{LL_BUILD}")
  set(CMAKE_XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL "${CMAKE_MATCH_1}")
  message(STATUS "CMAKE_XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL = '${CMAKE_XCODE_ATTRIBUTE_GCC_OPTIMIZATION_LEVEL}'")

  string(REGEX MATCHALL "[^ ]+" LL_BUILD_LIST "$ENV{LL_BUILD}")
  list(FIND LL_BUILD_LIST "-iwithsysroot" sysroot_idx)
  if ("${sysroot_idx}" LESS 0)
    message(FATAL_ERROR "Environment variable LL_BUILD must contain '-iwithsysroot'")
  endif ()
  math(EXPR sysroot_idx "${sysroot_idx} + 1")
  list(GET LL_BUILD_LIST "${sysroot_idx}" CMAKE_OSX_SYSROOT)
  message(STATUS "CMAKE_OSX_SYSROOT = '${CMAKE_OSX_SYSROOT}'")

  set(XCODE_VERSION 7.0)

  set(CMAKE_XCODE_ATTRIBUTE_GCC_VERSION "com.apple.compilers.llvm.clang.1_0")
  set(CMAKE_XCODE_ATTRIBUTE_GCC_STRICT_ALIASING NO)
  set(CMAKE_XCODE_ATTRIBUTE_GCC_FAST_MATH NO)
  set(CMAKE_XCODE_ATTRIBUTE_CLANG_X86_VECTOR_INSTRUCTIONS ssse3)

  set(CMAKE_OSX_ARCHITECTURES "${ARCH}")
  string(REPLACE "i686"  "i386"   CMAKE_OSX_ARCHITECTURES "${CMAKE_OSX_ARCHITECTURES}")
  string(REPLACE "AMD64" "x86_64" CMAKE_OSX_ARCHITECTURES "${CMAKE_OSX_ARCHITECTURES}")

  set(LL_ARCH ${ARCH}_darwin)
  set(LL_ARCH_DIR universal-darwin)
endif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

# Default deploy grid
set(GRID agni CACHE STRING "Target Grid")

set(VIEWER_CHANNEL "Second Life Test" CACHE STRING "Viewer Channel Name")

set(ENABLE_SIGNING OFF CACHE BOOL "Enable signing the viewer")
set(SIGNING_IDENTITY "" CACHE STRING "Specifies the signing identity to use, if necessary.")

set(VERSION_BUILD "0" CACHE STRING "Revision number passed in from the outside")
set(USESYSTEMLIBS OFF CACHE BOOL "Use libraries from your system rather than Linden-supplied prebuilt libraries.")
set(UNATTENDED OFF CACHE BOOL "Should be set to ON for building with VC Express editions.")

set(USE_PRECOMPILED_HEADERS ON CACHE BOOL "Enable use of precompiled header directives where supported.")

source_group("CMake Rules" FILES CMakeLists.txt)

endif(NOT DEFINED ${CMAKE_CURRENT_LIST_FILE}_INCLUDED)
