# -*- cmake -*-
include(Prebuilt)

include_guard()
create_target( ll::freetype)

use_prebuilt_binary(freetype)
set_target_include_dirs( ll::freetype  ${LIBS_PREBUILT_DIR}/include/freetype2/)
target_link_libraries( ll::freetype INTERFACE freetype )

