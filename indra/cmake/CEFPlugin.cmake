# -*- cmake -*-
include(Linking)
include(Prebuilt)

if(TARGET cef::cef)
    return()
endif()
create_target( cef::cef )

if (USESYSTEMLIBS)
    set(CEFPLUGIN OFF CACHE BOOL
        "CEFPLUGIN support for the llplugin/llmedia test apps.")
else (USESYSTEMLIBS)
    use_prebuilt_binary(dullahan)
    set_target_include_dirs( cef::cef ${LIBS_PREBUILT_DIR}/include/cef)
endif (USESYSTEMLIBS)

if (WINDOWS)
    set_target_libraries( cef::cef
        libcef.lib
        libcef_dll_wrapper.lib
        dullahan.lib
    )
elseif (DARWIN)
    FIND_LIBRARY(APPKIT_LIBRARY AppKit)
    if (NOT APPKIT_LIBRARY)
        message(FATAL_ERROR "AppKit not found")
    endif()

    set(CEF_LIBRARY "'${ARCH_PREBUILT_DIRS_RELEASE}/Chromium\ Embedded\ Framework.framework'")
    if (NOT CEF_LIBRARY)
        message(FATAL_ERROR "CEF not found")
    endif()

    set_target_libraries( cef::cef
        ${ARCH_PREBUILT_DIRS_RELEASE}/libcef_dll_wrapper.a
        ${ARCH_PREBUILT_DIRS_RELEASE}/libdullahan.a
        ${APPKIT_LIBRARY}
        "-F ${CEF_LIBRARY}"
       )

elseif (LINUX)
endif (WINDOWS)
