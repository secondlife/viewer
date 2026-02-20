# -*- cmake -*-

add_library(llxml STATIC)

target_sources(llxml
    PRIVATE
    llcontrol.cpp
    llxmlnode.cpp
    llxmlparser.cpp
    llxmltree.cpp
    )

target_sources(llxml
    PUBLIC
    FILE_SET HEADERS
    FILES
    llcontrol.h
    llxmlnode.h
    llxmlparser.h
    llxmltree.h
    )

# Libraries on which this library depends, needed for Linux builds
# Sort by high-level to low-level
target_link_libraries(llxml
    PUBLIC
    llfilesystem
    llmath
    llcommon
    ll::expat
    )
target_include_directories( llxml  INTERFACE   ${CMAKE_CURRENT_SOURCE_DIR})
target_precompile_headers(llxml REUSE_FROM llprecompiled)

# tests
if (BUILD_TESTING)
    # unit tests
    SET(llxml_TEST_SOURCE_FILES
      # none yet!
      )
    LL_ADD_PROJECT_UNIT_TESTS(llxml "${llxml_TEST_SOURCE_FILES}")

    # integration tests

    # set(TEST_DEBUG on)
    set(test_libs
            llxml
            llmath
            llcommon
            )
    set(test_project llxml)

    LL_ADD_INTEGRATION_TEST(llcontrol "" "${test_libs}" "${test_project}")
endif ()
