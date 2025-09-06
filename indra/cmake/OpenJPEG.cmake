# -*- cmake -*-
include_guard()

include(Prebuilt)
include(Linking)

add_library( ll::openjpeg INTERFACE IMPORTED )

use_system_binary(openjpeg)
use_prebuilt_binary(openjpeg)

find_library(OPENJPEG_LIBRARY
    NAMES
    openjp2
    openjp2.lib
    libopenjp2.a
    libopenjp2.so
    PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

target_link_libraries(ll::openjpeg INTERFACE ${OPENJPEG_LIBRARY})

target_include_directories(ll::openjpeg SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include/openjpeg)
