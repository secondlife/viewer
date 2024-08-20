# -*- cmake -*-
include(Prebuilt)

include_guard()
add_library( ll::expat INTERFACE IMPORTED )

use_system_binary(expat)
use_prebuilt_binary(expat)
if (WINDOWS)
    target_compile_definitions( ll::expat INTERFACE XML_STATIC=1)
    target_link_libraries( ll::expat  INTERFACE
            debug ${ARCH_PREBUILT_DIRS_DEBUG}/libexpatd.lib
            optimized ${ARCH_PREBUILT_DIRS_RELEASE}/libexpat.lib)
else ()
    target_link_libraries( ll::expat  INTERFACE
            debug ${ARCH_PREBUILT_DIRS_DEBUG}/libexpat.a
            optimized ${ARCH_PREBUILT_DIRS_RELEASE}/libexpat.a)
endif ()
target_include_directories( ll::expat SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include )
