# -*- cmake -*-
include(Linking)
include(Prebuilt)

if (USESYSTEMLIBS)
    set(LIBVLCPLUGIN OFF CACHE BOOL
        "LIBVLCPLUGIN support for the llplugin/llmedia test apps.")
else (USESYSTEMLIBS)
    set(LIBVLCPLUGIN ON CACHE BOOL
        "LIBVLCPLUGIN support for the llplugin/llmedia test apps.")
endif (USESYSTEMLIBS)

if (WINDOWS)
elseif (DARWIN)
elseif (LINUX)
endif (WINDOWS)
