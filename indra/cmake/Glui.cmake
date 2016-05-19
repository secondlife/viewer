# -*- cmake -*-
include(Linking)
include(Prebuilt)

if (USESYSTEMLIBS)
    set(GLUI OFF CACHE BOOL
        "GLUI support for the llplugin/llmedia test apps.")
else (USESYSTEMLIBS)
    use_prebuilt_binary(glui)
    set(GLUI ON CACHE BOOL
        "GLUI support for the llplugin/llmedia test apps.")
endif (USESYSTEMLIBS)

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
