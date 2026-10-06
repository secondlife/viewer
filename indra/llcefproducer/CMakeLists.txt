# -*- cmake -*-

# CEF headers require C++ 20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(00-Common)
include(LLCommon)
include(Linking)
include(EmbeddedBrowser)

project(llcefproducer)

# CEF only -- see indra/llvlcproducer/ for the separate LibVLC-only producer.
# Split 2026-09-30 for licensing reasons: the vendored vlc-bin package is
# declared GPL v2, while this project is LGPL v2.1 -- keeping libvlc out of
# this binary entirely (not just out of its own source) is the actual point
# of the split, so no LibVLCPlugin.cmake/ll::libvlc here, ever.
set(llcefproducer_SOURCE_FILES
    llcefproducer.cpp
    )

set(llcefproducer_HEADER_FILES
    CMakeLists.txt
    cefshm_protocol.h
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
    )

if (DARWIN)
    # Without this, CMake's default build-tree rpath (an absolute path into
    # this machine's own build directory, e.g. .../packages/lib/release) gets
    # baked into the binary as-is -- viewer_manifest.py only copies files,
    # it doesn't relink anything, so that absolute path would still be there
    # in the packaged .app, breaking the moment it's copied anywhere else (or
    # the build tree that happened to still exist at that exact path is
    # gone). Same fix newview/CMakeLists.txt already applies to the main
    # executable, just with the extra ../.. this target's own deeper deploy
    # location (Contents/Resources/SLCefProducer/SLCefProducer, see
    # viewer_manifest.py) needs to reach Contents/Frameworks -- where
    # viewer_manifest.py deposits the CEF framework this target needs.
    set_target_properties(SLCefProducer PROPERTIES
        BUILD_WITH_INSTALL_RPATH 1
        INSTALL_RPATH "@executable_path/../../Frameworks"
        )
elseif (LINUX)
    # Same underlying problem as Darwin's own rpath fix just above ($ORIGIN
    # is Linux's equivalent of @executable_path), but simpler here: no
    # bundle-shaped detour through a separate Frameworks-style directory --
    # SLCefProducer's libcef.so lives directly alongside it in the same
    # SLCefProducer/ directory (see viewer_manifest.py's LinuxManifest), so
    # plain $ORIGIN is enough.
    set_target_properties(SLCefProducer PROPERTIES
        BUILD_WITH_INSTALL_RPATH 1
        INSTALL_RPATH "$ORIGIN"
        )
endif (DARWIN)
