# -*- cmake -*-
include(Prebuilt)

add_library(ll::vhacd INTERFACE IMPORTED)

use_system_binary(vhacd)
use_prebuilt_binary(vhacd)

target_include_directories(ll::vhacd SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include/vhacd/)
