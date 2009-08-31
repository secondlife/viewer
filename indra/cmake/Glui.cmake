# -*- cmake -*-
include(Linking)
include(Prebuilt)

if (STANDALONE)
    set(GLUI OFF CACHE BOOL
        "GLUI support for the llplugin/llmedia test apps.")
else (STANDALONE)
    use_prebuilt_binary(glui)
    set(GLUI ON CACHE BOOL
        "GLUI support for the llplugin/llmedia test apps.")
endif (STANDALONE)

if (LINUX)
    set(GLUI ON CACHE BOOL
        "llplugin media apps HACK for Linux.")
endif (LINUX)

if (DARWIN OR LINUX)
    set(GLUI_LIBRARY
        glui)
endif (DARWIN OR LINUX)

if (WINDOWS)
    set(GLUI_LIBRARY
        debug glui32.lib
        optimized glui32.lib)
endif (WINDOWS)
