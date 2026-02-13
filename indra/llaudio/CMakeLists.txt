# -*- cmake -*-

add_library(llaudio STATIC)

target_sources(llaudio
    PRIVATE
    llaudioengine.cpp
    lllistener.cpp
    llaudiodecodemgr.cpp
    llvorbisencode.cpp
    )

target_sources(llaudio
    PUBLIC
    FILE_SET HEADERS
    FILES
    CMakeLists.txt

    llaudioengine.h
    lllistener.h
    llaudiodecodemgr.h
    llvorbisencode.h
    llwindgen.h
    )

target_include_directories( llaudio  INTERFACE   ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries( llaudio
        PUBLIC
        llcommon
        llmath
        llmessage
        llfilesystem
        ll::vorbis
    )

target_precompile_headers(llaudio REUSE_FROM llprecompiled)

if (TARGET ll::openal)
    target_sources(llaudio
        PRIVATE
        llaudioengine_openal.cpp
        lllistener_openal.cpp
        )

    target_sources(llaudio
        PUBLIC
        FILE_SET HEADERS
        FILES
        llaudioengine_openal.h
        lllistener_openal.h
        )

    target_link_libraries(llaudio PUBLIC ll::openal)
endif ()
