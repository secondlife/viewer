# -*- cmake -*-

# CEF headers require C++ 20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(00-Common)
include(LLCommon)
include(Linking)
include(EmbeddedBrowser)
include(LibVLCPlugin)

project(llmediaproducer)

set(llmediaproducer_SOURCE_FILES
    llmediaproducer.cpp
    libvlctabmanager.cpp
    )

set(llmediaproducer_HEADER_FILES
    CMakeLists.txt
    cefshm_protocol.h
    libvlctabmanager.h
    )

list(APPEND llmediaproducer_SOURCE_FILES ${llmediaproducer_HEADER_FILES})

if (WINDOWS)
    # Gives SLMediaProducer.exe a real VERSIONINFO resource (CompanyName, FileDescription,
    # etc.) -- without one, Task Manager, Explorer's Properties dialog, and some of
    # Windows' own security UI show a blank/"Unknown" identity for this exe. Mirrors
    # indra/newview/res/viewerRes.rc's own configure_file() pattern; VIEWER_VERSION_*
    # come from BuildVersion.cmake, already included by the time this subdirectory runs.
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/res/SLMediaProducerRes.rc
                    ${CMAKE_CURRENT_BINARY_DIR}/SLMediaProducerRes.rc
                    )
    list(APPEND llmediaproducer_SOURCE_FILES ${CMAKE_CURRENT_BINARY_DIR}/SLMediaProducerRes.rc)
endif (WINDOWS)

# WIN32: windowless by default (no console flash each session) -- see
# llmediaproducer.cpp's WinMain wrapper and its runtime --console opt-in.
add_executable(SLMediaProducer
    WIN32
    ${llmediaproducer_SOURCE_FILES}
    )

target_link_libraries(SLMediaProducer
    llcommon
    ll::shmframe
    ll::cefbrowser
    ll::libvlc
    )
