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

if (DARWIN)
    # Without this, CMake's default build-tree rpath (an absolute path into
    # this machine's own build directory, e.g. .../packages/lib/release) gets
    # baked into the binary as-is -- viewer_manifest.py only copies files,
    # it doesn't relink anything, so that absolute path would still be there
    # in the packaged .app, breaking the moment it's copied anywhere else (or
    # the build tree that happened to still exist at that exact path is
    # gone). Same fix newview/CMakeLists.txt already applies to the main
    # executable, just with the extra ../.. this target's own deeper deploy
    # location (Contents/Resources/SLMediaProducer/SLMediaProducer, see
    # viewer_manifest.py) needs to reach Contents/Frameworks -- where
    # viewer_manifest.py deposits libvlc*.dylib* (shared with the main exe,
    # which needs it directly linked in too for LLStreamingAudio_LibVLC).
    set_target_properties(SLMediaProducer PROPERTIES
        BUILD_WITH_INSTALL_RPATH 1
        INSTALL_RPATH "@executable_path/../../Frameworks"
        )
elseif (LINUX)
    # Same underlying problem as Darwin's own rpath fix just above ($ORIGIN
    # is Linux's equivalent of @executable_path), but simpler here: no
    # bundle-shaped detour through a separate Frameworks-style directory --
    # SLMediaProducer's libcef.so lives directly alongside it in the same
    # SLMediaProducer/ directory (see viewer_manifest.py's LinuxManifest), so
    # plain $ORIGIN is enough. libvlc itself needs no rpath entry at all here
    # -- unlike Windows/macOS, ll::libvlc on Linux links against the
    # system's own installed libvlc via pkg-config (see LibVLCPlugin.cmake),
    # not a vendored copy, so the dynamic linker's normal system search path
    # already finds it.
    set_target_properties(SLMediaProducer PROPERTIES
        BUILD_WITH_INSTALL_RPATH 1
        INSTALL_RPATH "$ORIGIN"
        )
endif (DARWIN)
