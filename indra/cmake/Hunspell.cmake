# -*- cmake -*-
include_guard()

add_library(ll::hunspell INTERFACE IMPORTED)
if(USE_VCPKG)
    find_package(PkgConfig REQUIRED)

    pkg_check_modules(hunspell REQUIRED IMPORTED_TARGET hunspell)
    target_link_libraries(ll::hunspell INTERFACE PkgConfig::hunspell)
    return()
endif()

include(Linking)
include(Prebuilt)

use_prebuilt_binary(dictionaries)

use_system_binary(hunspell)
use_prebuilt_binary(libhunspell)

if (WINDOWS)
    target_compile_definitions( ll::hunspell INTERFACE HUNSPELL_STATIC=1)
endif()

find_library(HUNSPELL_LIBRARY
    NAMES
    libhunspell.lib
    libhunspell-1.7.a
    PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

target_link_libraries(ll::hunspell INTERFACE ${HUNSPELL_LIBRARY})

target_include_directories( ll::hunspell SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include/hunspell)
