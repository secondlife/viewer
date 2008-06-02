# -*- cmake -*-

project(mac_crash_logger)

include(00-Common)
include(Boost)
include(LLCommon)
include(LLCrashLogger)
include(LLMath)
include(LLMessage)
include(LLVFS)
include(LLXML)
include(Linking)

include_directories(
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLCRASHLOGGER_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
    ${LLVFS_INCLUDE_DIRS}
    ${LLXML_INCLUDE_DIRS}
    )

set(mac_crash_logger_SOURCE_FILES
    mac_crash_logger.cpp
    llcrashloggermac.cpp
    )

set(mac_crash_logger_HEADER_FILES
    CMakeLists.txt

    llcrashloggermac.h
    )

set_source_files_properties(${mac_crash_logger_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND mac_crash_logger_SOURCE_FILES ${mac_crash_logger_HEADER_FILES})

add_executable(mac-crash-logger ${mac_crash_logger_SOURCE_FILES})

target_link_libraries(mac-crash-logger
    ${LLCRASHLOGGER_LIBRARIES}
    ${LLVFS_LIBRARIES}
    ${LLXML_LIBRARIES}
    ${LLMESSAGE_LIBRARIES}
    ${LLVFS_LIBRARIES}
    ${LLMATH_LIBRARIES}
    ${LLCOMMON_LIBRARIES}
    ${BOOST_SIGNALS_LIBRARY}
    )
