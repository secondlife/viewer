# -*- cmake -*-

project(llxuixml)

include(00-Common)
include(LLCommon)
include(LLMath)
include(LLXML)

include_directories(
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
    ${LLXML_INCLUDE_DIRS}
    )

set(llxuixml_SOURCE_FILES
    llinitparam.cpp
    lltrans.cpp
    lluicolor.cpp
    llxuiparser.cpp
    )
    
set(llxuixml_HEADER_FILES
    CMakeLists.txt

    llinitparam.h
    lltrans.h
    llregistry.h
    lluicolor.h
    llxuiparser.h
    )

set_source_files_properties(${llxuixml_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND llxuixml_SOURCE_FILES ${llxuixml_HEADER_FILES})

add_library (llxuixml ${llxuixml_SOURCE_FILES})
# Libraries on which this library depends, needed for Linux builds
# Sort by high-level to low-level
target_link_libraries(llxuixml
    llxml
    llcommon
    llmath
    )
