# -*- cmake -*-
include(Linking)
include(Prebuilt)

if (USESYSTEMLIBS)
    set(EXAMPLEPLUGIN OFF CACHE BOOL
        "EXAMPLEPLUGIN support for the llplugin/llmedia test apps.")
else (USESYSTEMLIBS)
    set(EXAMPLEPLUGIN ON CACHE BOOL
        "EXAMPLEPLUGIN support for the llplugin/llmedia test apps.")
endif (USESYSTEMLIBS)

if (WINDOWS)
elseif (DARWIN)
elseif (LINUX)
endif (WINDOWS)
