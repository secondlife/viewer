# -*- cmake -*-

project(llcrashlogger)

include(00-Common)
include(LLCoreHttp)
include(LLCommon)

set(llcrashlogger_SOURCE_FILES
    llcrashlogger.cpp
    llcrashlock.cpp
    )

set(llcrashlogger_HEADER_FILES
    CMakeLists.txt
    llcrashlogger.h
    llcrashlock.h
    )

set_source_files_properties(${llcrashlogger_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND llcrashlogger_SOURCE_FILES ${llcrashlogger_HEADER_FILES})

add_library(llcrashlogger ${llcrashlogger_SOURCE_FILES})
target_link_libraries( llcrashlogger llcommon llmessage llcorehttp llxml llfilesystem )
