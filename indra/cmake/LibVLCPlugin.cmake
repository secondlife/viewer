# -*- cmake -*-
include(Linking)
include(Prebuilt)

if( TARGET libvlc::libvlc )
    return()
endif()
create_target( libvlc::libvlc )

if (USESYSTEMLIBS)
    set(LIBVLCPLUGIN OFF CACHE BOOL
        "LIBVLCPLUGIN support for the llplugin/llmedia test apps.")
else (USESYSTEMLIBS)
    use_prebuilt_binary(vlc-bin)
    set(LIBVLCPLUGIN ON CACHE BOOL
        "LIBVLCPLUGIN support for the llplugin/llmedia test apps.")
        set(VLC_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/include/vlc)
endif (USESYSTEMLIBS)

if (WINDOWS)
    set_target_libraries( libvlc::libvlc
            libvlc.lib
            libvlccore.lib
    )
elseif (DARWIN)
    set_target_libraries( libvlc::libvlc
``      libvlc.dylib
        libvlccore.dylib
    )
elseif (LINUX)
    # Specify a full path to make sure we get a static link
    set_target_libraries( liblvc::libvlc
        ${LIBS_PREBUILT_DIR}/lib/libvlc.a
        ${LIBS_PREBUILT_DIR}/lib/libvlccore.a
    )
endif (WINDOWS)
