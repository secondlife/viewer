# -*- cmake -*-
include(Prebuilt)

include_guard()
create_target(ll::vorbis)

use_prebuilt_binary(ogg_vorbis)
set_target_include_dirs( ll::vorbis ${LIBS_PREBUILT_DIR}/include )

if (WINDOWS)
  set_target_libraries(ll::vorbis ogg_static vorbis_static vorbisenc_static vorbisfile_static )
else (WINDOWS)
  set_target_libraries(ll::vorbis ogg vorbis vorbisenc vorbisfile )
endif (WINDOWS)

