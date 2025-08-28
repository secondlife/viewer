# -*- cmake -*-

include(Prebuilt)

include_guard()
add_library( ll::entropycore INTERFACE IMPORTED )

# EntropyCore is a collection of different primitives that are useful for engine development
use_prebuilt_binary(EntropyCore)

if (WINDOWS)
    target_link_libraries( ll::entropycore INTERFACE ${ARCH_PREBUILT_DIRS_RELEASE}/EntropyCore.lib )
elseif (DARWIN)
    target_link_libraries( ll::entropycore INTERFACE ${ARCH_PREBUILT_DIRS_RELEASE}/libEntropyCore.a )
elseif (LINUX)
    target_link_libraries( ll::entropycore INTERFACE ${ARCH_PREBUILT_DIRS_RELEASE}/libEntropyCore.a )
endif (WINDOWS)

target_include_directories( ll::entropycore SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include/EntropyCore )