# -*- cmake -*-

project(win_updater)

include(00-Common)
include(LLCommon)
include(Linking)

include_directories(
    ${LLCOMMON_INCLUDE_DIRS}
    )

set(win_updater_SOURCE_FILES updater.cpp)

set(win_updater_HEADER_FILES CMakeLists.txt)

set_source_files_properties(${win_updater_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND win_updater_SOURCE_FILES ${win_updater_HEADER_FILES})

add_executable(windows-updater WIN32 ${win_updater_SOURCE_FILES})

target_link_libraries(windows-updater
    wininet
    user32
    gdi32
    shell32
    )

set_target_properties(windows-updater
    PROPERTIES
    LINK_FLAGS "/NODEFAULTLIB:LIBCMT"
    LINK_FLAGS_DEBUG "/NODEFAULTLIB:\"LIBCMT;LIBCMTD;MSVCRT\""
    )
