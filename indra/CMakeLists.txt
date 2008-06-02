# -*- cmake -*-

project(SecondLife)

# The FindPkgConfig module was released with 2.4.7.
cmake_minimum_required(VERSION 2.4.7 FATAL_ERROR)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(Variables)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Build type.  One of: Debug Release RelWithDebInfo" FORCE)
endif (NOT CMAKE_BUILD_TYPE)

add_subdirectory(cmake)

add_subdirectory(${LIBS_OPEN_PREFIX}llaudio)
add_subdirectory(${LIBS_OPEN_PREFIX}llcharacter)
add_subdirectory(${LIBS_OPEN_PREFIX}llcommon)
add_subdirectory(${LIBS_OPEN_PREFIX}llimage)
add_subdirectory(${LIBS_OPEN_PREFIX}llimagej2coj)
add_subdirectory(${LIBS_OPEN_PREFIX}llinventory)
add_subdirectory(${LIBS_OPEN_PREFIX}llmath)
add_subdirectory(${LIBS_OPEN_PREFIX}llmedia)
add_subdirectory(${LIBS_OPEN_PREFIX}llmessage)
add_subdirectory(${LIBS_OPEN_PREFIX}llprimitive)
add_subdirectory(${LIBS_OPEN_PREFIX}llrender)
add_subdirectory(${LIBS_OPEN_PREFIX}llvfs)
add_subdirectory(${LIBS_OPEN_PREFIX}llwindow)
add_subdirectory(${LIBS_OPEN_PREFIX}llxml)

if (EXISTS ${LIBS_CLOSED_DIR}llkdu AND NOT STANDALONE)
  add_subdirectory(${LIBS_CLOSED_PREFIX}llkdu)
endif (EXISTS ${LIBS_CLOSED_DIR}llkdu AND NOT STANDALONE)

add_subdirectory(${LIBS_OPEN_PREFIX}lscript)

if (WINDOWS)
    add_subdirectory(copy_win_scripts)
endif (WINDOWS)

if (VIEWER)
  add_subdirectory(${LIBS_OPEN_PREFIX}llcrashlogger)
  add_subdirectory(${LIBS_OPEN_PREFIX}llui)

  if (LINUX)
    add_subdirectory(${VIEWER_PREFIX}linux_crash_logger)
  elseif (DARWIN)
    add_subdirectory(${VIEWER_PREFIX}mac_crash_logger)
    add_subdirectory(${VIEWER_PREFIX}mac_updater)
  elseif (WINDOWS)
    add_subdirectory(${VIEWER_PREFIX}win_crash_logger)
    add_subdirectory(${VIEWER_PREFIX}win_updater)
  elseif (SOLARIS)
    add_subdirectory(solaris_crash_logger)
  endif (LINUX)

  add_subdirectory(${VIEWER_PREFIX}newview)
endif (VIEWER)

if (SERVER)
  include(${SERVER_PREFIX}Server.cmake)
endif (SERVER)
