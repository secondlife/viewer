# -*- cmake -*-
include(Linking)
include(Prebuilt)

include_guard()
add_library( ll::libvlc INTERFACE IMPORTED )

if(LINUX)
    find_package(PkgConfig REQUIRED)

    pkg_check_modules(libvlc REQUIRED IMPORTED_TARGET libvlc)
    target_link_libraries( ll::libvlc INTERFACE PkgConfig::libvlc)
    return()
endif()

use_prebuilt_binary(vlc-bin)
set(LIBVLCPLUGIN ON CACHE BOOL
        "LIBVLCPLUGIN support for the llplugin/llmedia test apps.")

if (WINDOWS)
    target_link_libraries( ll::libvlc INTERFACE
            libvlc.lib
            libvlccore.lib
    )
elseif (DARWIN)
    target_link_libraries( ll::libvlc INTERFACE
    ${ARCH_PREBUILT_DIRS_RELEASE}/libvlc.dylib
    ${ARCH_PREBUILT_DIRS_RELEASE}/libvlccore.dylib
    )
endif (WINDOWS)
