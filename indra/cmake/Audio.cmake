# -*- cmake -*-
include(Linking)
include(Prebuilt)

include_guard()
add_library( ll::vorbis INTERFACE IMPORTED )

use_system_binary(vorbis)
use_prebuilt_binary(ogg_vorbis)
target_include_directories( ll::vorbis SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include )

if (WINDOWS)
  target_link_libraries(ll::vorbis INTERFACE
        ${ARCH_PREBUILT_DIRS_RELEASE}/libogg.lib
        ${ARCH_PREBUILT_DIRS_RELEASE}/libvorbisenc.lib
        ${ARCH_PREBUILT_DIRS_RELEASE}/libvorbisfile.lib
        ${ARCH_PREBUILT_DIRS_RELEASE}/libvorbis.lib
    )
else (WINDOWS)
  target_link_libraries(ll::vorbis INTERFACE vorbisfile vorbis ogg vorbisenc )
endif (WINDOWS)

