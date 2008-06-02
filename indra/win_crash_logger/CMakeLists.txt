# -*- cmake -*-

project(win_crash_logger)

include(00-Common)
include(LLCommon)
include(LLCrashLogger)
include(LLMath)
include(LLMessage)
include(LLVFS)
include(LLWindow)
include(LLXML)
include(Linking)

include_directories(
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLCRASHLOGGER_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
    ${LLWINDOW_INCLUDE_DIRS}
    ${LLXML_INCLUDE_DIRS}
    ${LLVFS_INCLUDE_DIRS}
    )

set(win_crash_logger_SOURCE_FILES
    win_crash_logger.cpp
    llcrashloggerwindows.cpp
    )

set(win_crash_logger_HEADER_FILES
    CMakeLists.txt

    llcrashloggerwindows.h
    resource.h
    StdAfx.h
    win_crash_logger.h
    )

set_source_files_properties(${win_crash_logger_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND win_crash_logger_SOURCE_FILES ${win_crash_logger_HEADER_FILES})

find_library(DXGUID_LIBRARY dxguid ${DIRECTX_LIBRARY_DIR})

add_executable(windows-crash-logger WIN32 ${win_crash_logger_SOURCE_FILES})

target_link_libraries(windows-crash-logger
    ${LLCRASHLOGGER_LIBRARIES}
    ${LLWINDOW_LIBRARIES}
    ${LLVFS_LIBRARIES}
    ${LLXML_LIBRARIES}
    ${LLMESSAGE_LIBRARIES}
    ${LLMATH_LIBRARIES}
    ${LLCOMMON_LIBRARIES}
    ${WINDOWS_LIBRARIES}
    ${DXGUID_LIBRARY}
    )
    
if (WINDOWS)
    set_target_properties(windows-crash-logger
        PROPERTIES 
        LINK_FLAGS "/NODEFAULTLIB:MSVCRT"
        LINK_FLAGS_DEBUG "/NODEFAULTLIB:LIBCMT /NODEFAULTLIB:MSVCRTD"
        )
endif (WINDOWS)
