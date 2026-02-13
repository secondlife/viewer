# -*- cmake -*-
include_guard()
add_library(ll::sse2neon INTERFACE IMPORTED)

if(USE_VCPKG AND DARWIN)
    find_path(SSE2NEON_INCLUDE_DIRS "sse2neon/sse2neon.h")
    target_include_directories(ll::sse2neon SYSTEM INTERFACE ${SSE2NEON_INCLUDE_DIRS})
    return()
endif()

if (DARWIN)
    include(Prebuilt)
    use_system_binary(sse2neon)
    use_prebuilt_binary(sse2neon)

    target_include_directories( ll::sse2neon SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include/sse2neon)
endif()
