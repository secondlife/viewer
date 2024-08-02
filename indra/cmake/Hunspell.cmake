# -*- cmake -*-
include(Linking)
include(Prebuilt)

include_guard()
use_prebuilt_binary(dictionaries)

add_library( ll::hunspell INTERFACE IMPORTED )
use_system_binary(hunspell)
use_prebuilt_binary(libhunspell)
if (WINDOWS)
    target_compile_definitions( ll::hunspell INTERFACE HUNSPELL_STATIC=1)
    target_link_libraries( ll::hunspell INTERFACE
        debug ${ARCH_PREBUILT_DIRS_DEBUG}/libhunspell.lib
        optimized ${ARCH_PREBUILT_DIRS_RELEASE}/libhunspell.lib
        )
elseif(DARWIN)
    target_link_libraries( ll::hunspell INTERFACE ${ARCH_PREBUILT_DIRS_RELEASE}/libhunspell-1.7.a
        )
elseif(LINUX)
    target_link_libraries( ll::hunspell INTERFACE ${ARCH_PREBUILT_DIRS_RELEASE}/libhunspell-1.7.a
        )
endif()
target_include_directories( ll::hunspell SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include/hunspell)
