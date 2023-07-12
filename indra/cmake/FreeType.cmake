# -*- cmake -*-
include(Prebuilt)

include_guard()
add_library( ll::freetype INTERFACE IMPORTED )

use_system_binary(freetype)
use_prebuilt_binary(freetype)
target_include_directories( ll::freetype SYSTEM INTERFACE  ${LIBS_PREBUILT_DIR}/include/freetype2/)
target_link_libraries( ll::freetype INTERFACE freetype )

