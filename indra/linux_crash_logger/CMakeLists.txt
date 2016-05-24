# -*- cmake -*-

project(linux_crash_logger)

include(00-Common)
include(GLH)
include(LLCoreHttp)
include(LLCommon)
include(LLCrashLogger)
include(LLMath)
include(LLMessage)
include(LLVFS)
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
    ${LLVFS_INCLUDE_DIRS}
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

set_source_files_properties(${linux_crash_logger_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND linux_crash_logger_SOURCE_FILES
     ${linux_crash_logger_HEADER_FILES}
     )

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--as-needed")

add_executable(linux-crash-logger ${linux_crash_logger_SOURCE_FILES})

# llcommon uses `clock_gettime' which is provided by librt on linux.
set(LIBRT_LIBRARY rt)


target_link_libraries(linux-crash-logger
    ${LLCRASHLOGGER_LIBRARIES}
    ${LLVFS_LIBRARIES}
    ${LLXML_LIBRARIES}
    ${LLMESSAGE_LIBRARIES}
    ${LLVFS_LIBRARIES}
    ${LLMATH_LIBRARIES}
    ${LLCOREHTTP_LIBRARIES}
    ${LLCOMMON_LIBRARIES}
    ${BOOST_COROUTINE_LIBRARY}
    ${BOOST_CONTEXT_LIBRARY}
    ${UI_LIBRARIES}
    ${DB_LIBRARIES}
    ${FREETYPE_LIBRARIES}
    ${LIBRT_LIBRARY}
    )

add_custom_target(linux-crash-logger-target ALL
                  DEPENDS linux-crash-logger)
