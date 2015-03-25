# -*- cmake -*-
include(Linking)
include(Prebuilt)

if (USESYSTEMLIBS)
    set(CEFPLUGIN OFF CACHE BOOL
        "CEFPLUGIN support for the llplugin/llmedia test apps.")
else (USESYSTEMLIBS)
    use_prebuilt_binary(cef)
    set(CEFPLUGIN ON CACHE BOOL
        "CEFPLUGIN support for the llplugin/llmedia test apps.")
        set(CEF_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/include/cef)
endif (USESYSTEMLIBS)

if (WINDOWS)
elseif (DARWIN)
elseif (LINUX)
endif (WINDOWS)
