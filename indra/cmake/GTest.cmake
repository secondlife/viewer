# -*- cmake -*-
#
# GoogleTest integration for the Second Life viewer.
#
# Pulls gtest in via FetchContent so we don't need an autobuild package.
# Used for *new* tests written for this fork; legacy tests still use TUT.
#
# Provides:
#   gtest        — gtest static lib
#   gtest_main   — gtest with default main()
#   gmock, gmock_main — googlemock counterparts
#
# Usage from a library CMakeLists.txt:
#   include(GTest)
#   include(LLAddGTest)
#   LL_ADD_GTEST(llmath llmatrix4_gtest "${test_libs}")
#
# The test source goes in `tests_gtest/<name>.cpp` next to the legacy
# `tests/` TUT directory.

if (LL_TESTS AND NOT TARGET gtest)
  include(FetchContent)

  # GoogleTest 1.15.2 — last release supporting C++17 minimum
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
    GIT_SHALLOW    TRUE
  )

  # Don't install gtest, don't pull in gmock unless we ask for it.
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  set(BUILD_GMOCK   ON  CACHE BOOL "" FORCE)
  # Match the rest of the viewer's runtime library setting on Windows.
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

  FetchContent_MakeAvailable(googletest)

  # Group gtest's targets in IDE folders for tidiness.
  set_target_properties(gtest      PROPERTIES FOLDER "ThirdParty/gtest")
  set_target_properties(gtest_main PROPERTIES FOLDER "ThirdParty/gtest")
  if (TARGET gmock)
    set_target_properties(gmock      PROPERTIES FOLDER "ThirdParty/gtest")
    set_target_properties(gmock_main PROPERTIES FOLDER "ThirdParty/gtest")
  endif()
endif()
