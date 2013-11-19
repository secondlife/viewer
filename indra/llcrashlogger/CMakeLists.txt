# -*- cmake -*-

project(llcrashlogger)

include(00-Common)
include(LLCommon)
include(LLMath)
include(LLMessage)
include(LLVFS)
include(LLXML)

include_directories(
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
    ${LLMESSAGE_INCLUDE_DIRS}
    ${LLVFS_INCLUDE_DIRS}
    ${LLXML_INCLUDE_DIRS}
    )
include_directories(SYSTEM
    ${LLCOMMON_SYSTEM_INCLUDE_DIRS}
    ${LLXML_SYSTEM_INCLUDE_DIRS}
    )

set(llcrashlogger_SOURCE_FILES
    llcrashlogger.cpp
    )

set(llcrashlogger_HEADER_FILES
    CMakeLists.txt

    llcrashlogger.h
    )

set_source_files_properties(${llcrashlogger_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND llcrashlogger_SOURCE_FILES ${llcrashlogger_HEADER_FILES})

add_library(llcrashlogger ${llcrashlogger_SOURCE_FILES})
