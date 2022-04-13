# -*- cmake -*-
include(Prebuilt)

if( TARGET freetype::freetype )
  return()
endif()
create_target( freetype::freetype)

use_prebuilt_binary(freetype)
set_target_include_dirs( freetype::freetype  ${LIBS_PREBUILT_DIR}/include/freetype2/)
set_target_libraries( freetype::freetype freetype )

