# -*- cmake -*-

# CEF headers require C++ 20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(00-Common)
include(LLCommon)
include(Linking)
include(EmbeddedBrowser)

project(llcefproducer)

set(llcefproducer_SOURCE_FILES
    llcefproducer.cpp
    )

set(llcefproducer_HEADER_FILES
    CMakeLists.txt
    cefshm_protocol.h
    )

list(APPEND llcefproducer_SOURCE_FILES ${llcefproducer_HEADER_FILES})

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
    )
