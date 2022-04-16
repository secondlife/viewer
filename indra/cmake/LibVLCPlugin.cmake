# -*- cmake -*-
include(Linking)
include(Prebuilt)

include_guard()
create_target( ll::libvlc )

use_prebuilt_binary(vlc-bin)
set(LIBVLCPLUGIN ON CACHE BOOL
        "LIBVLCPLUGIN support for the llplugin/llmedia test apps.")

if (WINDOWS)
    set_target_libraries( ll::libvlc
            libvlc.lib
            libvlccore.lib
    )
elseif (DARWIN)
    set_target_libraries( ll::libvlc
            libvlc.dylib
            libvlccore.dylib
    )
elseif (LINUX)
    # Specify a full path to make sure we get a static link
    set_target_libraries( ll::libvlc
        ${LIBS_PREBUILT_DIR}/lib/libvlc.a
        ${LIBS_PREBUILT_DIR}/lib/libvlccore.a
    )
endif (WINDOWS)
