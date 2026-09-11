# -*- cmake -*-
include(Linking)
include(Prebuilt)

include_guard()
add_library( ll::libvlc INTERFACE IMPORTED )

set(LIBVLCPLUGIN ON CACHE BOOL
        "LIBVLCPLUGIN support for the llplugin/llmedia test apps.")

if (LIBVLCPLUGIN)
    if(LINUX)
        find_package(PkgConfig REQUIRED)

        pkg_check_modules(libvlc REQUIRED IMPORTED_TARGET libvlc)
        target_link_libraries( ll::libvlc INTERFACE PkgConfig::libvlc)
        return()
    endif()

    use_prebuilt_binary(vlc-bin)

    find_library(VLC_LIBRARY
        NAMES
        libvlc.lib
        libvlc.dylib
        PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

    find_library(VLCCORE_LIBRARY
        NAMES
        libvlccore.lib
        libvlccore.dylib
        PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

    target_link_libraries(ll::libvlc INTERFACE ${VLC_LIBRARY} ${VLCCORE_LIBRARY})
endif()
