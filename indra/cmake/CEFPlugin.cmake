# -*- cmake -*-
include(Linking)
include(Prebuilt)

if (USESYSTEMLIBS)
    set(CEFPLUGIN OFF CACHE BOOL
        "CEFPLUGIN support for the llplugin/llmedia test apps.")
else (USESYSTEMLIBS)
    set(CEFPLUGIN ON CACHE BOOL
        "CEFPLUGIN support for the llplugin/llmedia test apps.")
endif (USESYSTEMLIBS)

if (WINDOWS)
elseif (DARWIN)
elseif (LINUX)
endif (WINDOWS)
