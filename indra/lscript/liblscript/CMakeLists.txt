# -*- cmake -*-

project(liblscript)

include(00-Common)
include(LLCommon)
include(LLMath)
include(LScript)

set(liblscript_SOURCE_FILES
    ../lscript_library/lscript_alloc.cpp
    ../lscript_library/lscript_export.cpp
    ../lscript_library/lscript_library.cpp
    ../lscript_execute/llscriptresource.cpp
    ../lscript_execute/llscriptresourceconsumer.cpp
    ../lscript_execute/llscriptresourcepool.cpp
    ../lscript_execute/lscript_execute.cpp
    ../lscript_execute/lscript_heapruntime.cpp
    ../lscript_execute/lscript_readlso.cpp
    )

set(liblscript_HEADER_FILES
    CMakeLists.txt
    liblscript.h
    ../llscriptresource.h
    ../llscriptresourceconsumer.h
    ../llscriptresourcepool.h
    ../lscript_alloc.h
    ../lscript_byteconvert.h
    ../lscript_byteformat.h
    ../lscript_execute.h
    ../lscript_export.h
    ../lscript_library.h
    ../lscript_rt_interface.h
    )

set_source_files_properties(${liblscript_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND liblscript_SOURCE_FILES ${liblscript_HEADER_FILES})

add_library(liblscript STATIC ${liblscript_SOURCE_FILES})

target_include_directories(liblscript
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
    PRIVATE
        ${LLCOMMON_INCLUDE_DIRS}
        ${LLMATH_INCLUDE_DIRS}
        ${LSCRIPT_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/../lscript_compile
        ${CMAKE_CURRENT_SOURCE_DIR}/../lscript_execute
        ${CMAKE_CURRENT_SOURCE_DIR}/../lscript_library
        ${CMAKE_SOURCE_DIR}/llcommon
        ${CMAKE_SOURCE_DIR}/llmath
    )
target_include_directories(liblscript SYSTEM PRIVATE
    ${LLCOMMON_SYSTEM_INCLUDE_DIRS}
    )

target_link_libraries(liblscript
    PUBLIC
        llcommon
        llmath
    )

set_target_properties(liblscript PROPERTIES
    PUBLIC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/liblscript.h;${CMAKE_CURRENT_SOURCE_DIR}/../lscript_library.h;${CMAKE_CURRENT_SOURCE_DIR}/../lscript_execute.h;${CMAKE_SOURCE_DIR}/llcommon/lluuid.h"
    )
