# -*- cmake -*-
#
# Shared GoogleTest support for viewer unit tests.
#
# This directory provides:
#   * GoogleTest / GoogleMock, built from a pinned upstream release.
#   * llgtest_main - a static library containing main() plus common test
#     helpers (llgtest.h).  Every gtest executable links against it.
#
# Tests are declared with ll_add_gtest() from indra/cmake/LLAddGTest.cmake.

project(llgtest)

include(FetchContent)

# Pull in googletest *before* including 00-Common so that the viewer's
# directory-level compile options (/WX, -Werror, etc.) are not applied to
# third-party code.
#
# The source is always built here, never taken from find_package(), so every
# test links a gtest compiled with the viewer's compiler, standard and CRT.
#
# To build offline, point FETCHCONTENT_SOURCE_DIR_GOOGLETEST at an unpacked
# copy of the same release.
set(LL_GOOGLETEST_VERSION "1.17.0")

if (POLICY CMP0135)
    # Extracted files get the extraction time as their timestamp.
    cmake_policy(SET CMP0135 NEW)
endif ()

FetchContent_Declare(
    googletest
    URL      https://github.com/google/googletest/archive/refs/tags/v${LL_GOOGLETEST_VERSION}.tar.gz
    URL_HASH SHA256=65fab701d9829d38cb77c14acdc431d2108bfdbf8979e40eb8ae567edf10b27c
    )

# Match the viewer's dynamic CRT (/MD) on Windows rather than gtest's /MT default.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

foreach (gtest_target gtest gtest_main gmock gmock_main)
    if (TARGET ${gtest_target})
        set_target_properties(${gtest_target} PROPERTIES FOLDER "Tests/googletest")
        if (DARWIN)
            set_target_properties(${gtest_target} PROPERTIES OSX_ARCHITECTURES ${LL_MACOS_TEST_ARCHITECTURE})
        endif ()
    endif ()
endforeach ()

include(00-Common)

set(llgtest_SOURCE_FILES
    llgtest_main.cpp
    )

set(llgtest_HEADER_FILES
    CMakeLists.txt
    llgtest.h
    )

list(APPEND llgtest_SOURCE_FILES ${llgtest_HEADER_FILES})

add_library(llgtest_main STATIC ${llgtest_SOURCE_FILES})
target_include_directories(llgtest_main PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(llgtest_main PUBLIC GTest::gtest GTest::gmock)
set_target_properties(llgtest_main PROPERTIES FOLDER "Tests")

if (WINDOWS)
    target_link_libraries(llgtest_main PUBLIC dbghelp) # crash stack traces, see llgtest_main.cpp
endif ()

if (DARWIN)
    set_target_properties(llgtest_main PROPERTIES OSX_ARCHITECTURES ${LL_MACOS_TEST_ARCHITECTURE})
endif ()
