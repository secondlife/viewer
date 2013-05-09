# -*- cmake -*-

project(win_updater)

include(00-Common)
include(LLCommon)
include(Linking)

# *HACK - override msvcrt implementation (intialized on 00-Common) to be
# statically linked for the installer this relies on vc taking the last flag on
# the command line
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MT")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")

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
    LINK_FLAGS "/NODEFAULTLIB:MSVCRT"
    LINK_FLAGS_DEBUG "/NODEFAULTLIB:\"LIBCMT;MSVCRT\""
    )

# The windows-updater doesn't link against anything non-system, apparently
#ll_deploy_sharedlibs_command(windows-updater)
