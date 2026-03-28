# -*- cmake -*-

project(llxml)

include(00-Common)
include(LLCommon)

set(llxml_SOURCE_FILES
    llcontrol.cpp
    llxmlnode.cpp
    llxmlparser.cpp
    llxmltree.cpp
    )

set(llxml_HEADER_FILES
    CMakeLists.txt

    llcontrol.h
    llxmlnode.h
    llxmlparser.h
    llxmltree.h
    )

list(APPEND llxml_SOURCE_FILES ${llxml_HEADER_FILES})

add_library (llxml ${llxml_SOURCE_FILES})
# Libraries on which this library depends, needed for Linux builds
# Sort by high-level to low-level
target_link_libraries( llxml
        llfilesystem
        llmath
        llcommon
        ll::expat
    )
target_include_directories( llxml  INTERFACE   ${CMAKE_CURRENT_SOURCE_DIR})


# tests

if (LL_TESTS)
    include(Doctest)
    add_subdirectory(tests_doctest)
endif (LL_TESTS)
