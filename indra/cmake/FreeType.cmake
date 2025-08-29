# -*- cmake -*-
include(Prebuilt)
include(Linking)

include_guard()
add_library( ll::freetype INTERFACE IMPORTED )

use_system_binary(freetype)
use_prebuilt_binary(freetype)
target_include_directories( ll::freetype SYSTEM INTERFACE  ${LIBS_PREBUILT_DIR}/include/freetype2/)

find_library(FREETYPE_LIBRARY
    NAMES
    freetype.lib
    libfreetype.a
    PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

target_link_libraries(ll::freetype INTERFACE ${FREETYPE_LIBRARY})
