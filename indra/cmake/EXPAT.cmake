# -*- cmake -*-
include(Prebuilt)

include_guard()
add_library(ll::expat INTERFACE IMPORTED)

use_system_binary(expat)
use_prebuilt_binary(expat)

if (WINDOWS)
    target_compile_definitions(ll::expat INTERFACE XML_STATIC=1)
endif ()

find_library(EXPAT_LIBRARY
    NAMES
    libexpat.lib
    libexpat.a
    PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

target_link_libraries(ll::expat INTERFACE ${EXPAT_LIBRARY})

target_include_directories(ll::expat SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include)
