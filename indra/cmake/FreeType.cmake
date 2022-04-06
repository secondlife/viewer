# -*- cmake -*-
include(Prebuilt)

if( TARGET freetype::freetype )
  return()
endif()
create_target( freetype::freetype)

if (USESYSTEMLIBS)
  include(FindPkgConfig)

  pkg_check_modules(FREETYPE REQUIRED freetype2)
else (USESYSTEMLIBS)
  use_prebuilt_binary(freetype)
  set_target_include_dirs( freetype::freetype  ${LIBS_PREBUILT_DIR}/include/freetype2/)
  set_target_libraries( freetype::freetype freetype )
endif (USESYSTEMLIBS)

