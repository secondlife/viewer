# -*- cmake -*-

# CEF headers require C++ 20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(00-Common)
include(LLCommon)
include(Linking)
include(EmbeddedBrowser)
include(LibVLCPlugin)

project(llcefproducer)

set(llcefproducer_SOURCE_FILES
    llcefproducer.cpp
    libvlctabmanager.cpp
    )

set(llcefproducer_HEADER_FILES
    CMakeLists.txt
    cefshm_protocol.h
    libvlctabmanager.h
    )

list(APPEND llcefproducer_SOURCE_FILES ${llcefproducer_HEADER_FILES})

if (WINDOWS)
    # Gives SLCefProducer.exe a real VERSIONINFO resource (CompanyName, FileDescription,
    # etc.) -- without one, Task Manager, Explorer's Properties dialog, and some of
    # Windows' own security UI show a blank/"Unknown" identity for this exe. Mirrors
    # indra/newview/res/viewerRes.rc's own configure_file() pattern; VIEWER_VERSION_*
    # come from BuildVersion.cmake, already included by the time this subdirectory runs.
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/res/SLCefProducerRes.rc
                    ${CMAKE_CURRENT_BINARY_DIR}/SLCefProducerRes.rc
                    )
    list(APPEND llcefproducer_SOURCE_FILES ${CMAKE_CURRENT_BINARY_DIR}/SLCefProducerRes.rc)
endif (WINDOWS)

# WIN32: windowless by default (no console flash each session) -- see
# llcefproducer.cpp's WinMain wrapper and its runtime --console opt-in.
add_executable(SLCefProducer
    WIN32
    ${llcefproducer_SOURCE_FILES}
    )

target_link_libraries(SLCefProducer
    llcommon
    ll::shmframe
    ll::cefbrowser
    ll::libvlc
    )
