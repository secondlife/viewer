# -*- cmake -*-

project(llxml)

include(00-Common)
include(LLCommon)
include(LLMath)
include(LLXML)

include_directories(
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
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
    llxmlnode.h
    llxmlparser.h
    llxmltree.h
    )

set_source_files_properties(${llxml_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND llxml_SOURCE_FILES ${llxml_HEADER_FILES})

add_library (llxml ${llxml_SOURCE_FILES})
target_link_libraries(
    llxml
    ${BOOST_SIGNALS_LIBRARY}
    ${EXPAT_LIBRARIES}
    )
