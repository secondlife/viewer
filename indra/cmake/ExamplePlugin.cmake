# -*- cmake -*-
include(Linking)
include(Prebuilt)

if (STANDALONE)
    set(EXAMPLEPLUGIN OFF CACHE BOOL
        "EXAMPLEPLUGIN support for the llplugin/llmedia test apps.")
else (STANDALONE)
    set(EXAMPLEPLUGIN ON CACHE BOOL
        "EXAMPLEPLUGIN support for the llplugin/llmedia test apps.")
endif (STANDALONE)

if (WINDOWS)
elseif (DARWIN)
elseif (LINUX)
endif (WINDOWS)
