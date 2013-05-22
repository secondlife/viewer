# -*- cmake -*-

project(llxml)

include(00-Common)
include(LLCommon)
include(LLMath)
include(LLVFS)
include(LLXML)

include_directories(
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
    ${LLVFS_INCLUDE_DIRS}
    )
include_directories(
    ${LLCOMMON_SYSTEM_INCLUDE_DIRS}
    )

set(llxml_SOURCE_FILES
    llcontrol.cpp
    llxmlnode.cpp
    llxmlparser.cpp
    llxmltree.cpp
    )

set(llxml_HEADER_FILES
    CMakeLists.txt

    llcontrol.h
    llcontrolgroupreader.h
    llxmlnode.h
    llxmlparser.h
    llxmltree.h
    )

set_source_files_properties(${llxml_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND llxml_SOURCE_FILES ${llxml_HEADER_FILES})

add_library (llxml ${llxml_SOURCE_FILES})
# Libraries on which this library depends, needed for Linux builds
# Sort by high-level to low-level
target_link_libraries(llxml
    ${LLVFS_LIBRARIES}
    ${LLMATH_LIBRARIES}
    ${LLCOMMON_LIBRARIES}
    ${EXPAT_LIBRARIES}
    )

# tests

if (LL_TESTS)
    # unit tests

    SET(llxml_TEST_SOURCE_FILES
      # none yet!
      )
    LL_ADD_PROJECT_UNIT_TESTS(llxml "${llxml_TEST_SOURCE_FILES}")

    # integration tests

    #    set(TEST_DEBUG on)
    set(test_libs
      ${LLXML_LIBRARIES}
      ${WINDOWS_LIBRARIES}
      ${LLMATH_LIBRARIES}
      ${LLCOMMON_LIBRARIES}
      )

    LL_ADD_INTEGRATION_TEST(llcontrol "" "${test_libs}")
endif (LL_TESTS)
