# -*- cmake -*-

project(linux_crash_logger)

include(00-Common)
include(GLH)
include(LLCoreHttp)
include(LLCommon)
include(LLCrashLogger)
include(LLMath)
include(LLMessage)
include(LLFileSystem)
include(LLXML)
include(Linking)
include(UI)
include(FreeType)
include(Boost)

include_directories(
    ${LLCOREHTTP_INCLUDE_DIRS}
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLCRASHLOGGER_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
    ${LLFILESYSTEM_INCLUDE_DIRS}
    ${LLXML_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
    )
include_directories(SYSTEM
    ${LLCOMMON_SYSTEM_INCLUDE_DIRS}
    ${LLXML_SYSTEM_INCLUDE_DIRS}
    )
include_directories(SYSTEM
    ${LLCOMMON_SYSTEM_INCLUDE_DIRS}
    ${LLXML_SYSTEM_INCLUDE_DIRS}
    )

set(linux_crash_logger_SOURCE_FILES
    linux_crash_logger.cpp
    llcrashloggerlinux.cpp
    )

set(linux_crash_logger_HEADER_FILES
    CMakeLists.txt

    llcrashloggerlinux.h
    )

list(APPEND linux_crash_logger_SOURCE_FILES
     ${linux_crash_logger_HEADER_FILES}
     )

add_executable(linux-crash-logger ${linux_crash_logger_SOURCE_FILES})

# llcommon uses `clock_gettime' which is provided by librt on linux.
set(LIBRT_LIBRARY rt)


target_link_libraries(linux-crash-logger
        llcrashlogger
        llfilesystem
        llxml
        llmessage
        llmath
        llcorehttp
        llcommon
        ll::uilibraries
    )

add_custom_target(linux-crash-logger-target ALL
    DEPENDS linux-crash-logger)
