# -*- cmake -*-

include(00-Common)
include(LLCommon)
include(Linking)
include(EmbeddedBrowser)
include(LibVLCPlugin)

project(llvlcproducer)

# LibVLC only -- see indra/llcefproducer/ for the separate CEF-only producer.
# Split from the former combined SLMediaProducer 2026-09-30 for licensing
# reasons: the vendored vlc-bin package is declared GPL v2, while this
# project is LGPL v2.1 -- keeping libvlc confined to this one small,
# standalone process (reached only via IPC) is the actual point of the
# split, so no EmbeddedBrowser.cmake's ll::cefbrowser/CEF headers here, ever.
set(llvlcproducer_SOURCE_FILES
    llvlcproducer.cpp
    libvlctabmanager.cpp
    )

set(llvlcproducer_HEADER_FILES
    CMakeLists.txt
    cefshm_protocol.h
    libvlctabmanager.h
    )

list(APPEND llvlcproducer_SOURCE_FILES ${llvlcproducer_HEADER_FILES})

if (WINDOWS)
    # Gives SLVlcProducer.exe a real VERSIONINFO resource (CompanyName, FileDescription,
    # etc.) -- without one, Task Manager, Explorer's Properties dialog, and some of
    # Windows' own security UI show a blank/"Unknown" identity for this exe. Mirrors
    # indra/llcefproducer/CMakeLists.txt's own identical pattern.
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/res/SLVlcProducerRes.rc
                    ${CMAKE_CURRENT_BINARY_DIR}/SLVlcProducerRes.rc
                    )
    list(APPEND llvlcproducer_SOURCE_FILES ${CMAKE_CURRENT_BINARY_DIR}/SLVlcProducerRes.rc)
endif (WINDOWS)

# WIN32: windowless by default (no console flash each session) -- see
# llvlcproducer.cpp's WinMain wrapper and its runtime --console opt-in.
add_executable(SLVlcProducer
    WIN32
    ${llvlcproducer_SOURCE_FILES}
    )

target_link_libraries(SLVlcProducer
    llcommon
    ll::shmframe
    ll::libvlc
    )

if (DARWIN)
    # libvlc*.dylib* is bundled directly alongside this executable (see
    # viewer_manifest.py's DarwinManifest SLVlcProducer block), not in a
    # separate Frameworks-style directory the way CEF needs (no per-sub-process
    # helper bundles, no framework resources to reach) -- so plain
    # @executable_path is enough, no ../.. detour needed.
    set_target_properties(SLVlcProducer PROPERTIES
        BUILD_WITH_INSTALL_RPATH 1
        INSTALL_RPATH "@executable_path"
        )
elseif (LINUX)
    # Same reasoning as Darwin just above ($ORIGIN is Linux's equivalent of
    # @executable_path) -- except Linux needs no rpath entry for libvlc at
    # all: ll::libvlc there links against the system's own installed libvlc
    # via pkg-config (see LibVLCPlugin.cmake), not a vendored copy, so the
    # dynamic linker's normal system search path already finds it. Set
    # anyway for consistency/future-proofing, same as llcefproducer does.
    set_target_properties(SLVlcProducer PROPERTIES
        BUILD_WITH_INSTALL_RPATH 1
        INSTALL_RPATH "$ORIGIN"
        )
endif (DARWIN)
