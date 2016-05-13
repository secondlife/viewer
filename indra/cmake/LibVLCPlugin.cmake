# -*- cmake -*-
include(Linking)
include(Prebuilt)

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
    set(VLC_PLUGIN_LIBRARIES
        libvlc.lib
        libvlccore.lib
    )
elseif (DARWIN)
elseif (LINUX)
endif (WINDOWS)
