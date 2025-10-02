# -*- cmake -*-
include(Prebuilt)

include_guard()
add_library( ll::libpng INTERFACE IMPORTED )

use_system_binary(libpng)
use_prebuilt_binary(libpng)

find_library(LIBPNG_LIBRARY
    NAMES
    libpng16.lib
    libpng16.a
    PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

target_link_libraries(ll::libpng INTERFACE ${LIBPNG_LIBRARY})
target_include_directories(ll::libpng SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include/libpng16)
