# -*- cmake -*-
include(Prebuilt)

if(TARGET vorbis::vorbis)
  return()
endif()
create_target(vorbis::vorbis)

if (USESYSTEMLIBS)
  include(FindPkgConfig)
  pkg_check_modules(OGG REQUIRED ogg)
  pkg_check_modules(VORBIS REQUIRED vorbis)
  pkg_check_modules(VORBISENC REQUIRED vorbisenc)
  pkg_check_modules(VORBISFILE REQUIRED vorbisfile)
else (USESYSTEMLIBS)
  use_prebuilt_binary(ogg_vorbis)
  set_target_include_dirs( vorbis::vorbis ${LIBS_PREBUILT_DIR}/include )

  if (WINDOWS)
    set_target_libraries(vorbis::vorbis ogg_static vorbis_static vorbisenc_static vorbisfile_static )
  else (WINDOWS)
    set_target_libraries(vorbis::vorbis ogg vorbis vorbisenc vorbisfile )
  endif (WINDOWS)
endif (USESYSTEMLIBS)

