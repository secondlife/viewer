# -*- cmake -*-
include(Prebuilt)
include(Linking)

include_guard()
add_library( ll::freetype INTERFACE IMPORTED )

use_system_binary(freetype)
use_prebuilt_binary(freetype)
target_include_directories( ll::freetype SYSTEM INTERFACE  ${LIBS_PREBUILT_DIR}/include/freetype2/)

if (WINDOWS)
    target_link_libraries( ll::freetype INTERFACE ${ARCH_PREBUILT_DIRS_RELEASE}/freetype.lib)
else()
    target_link_libraries( ll::freetype INTERFACE ${ARCH_PREBUILT_DIRS_RELEASE}/libfreetype.a)
endif()

