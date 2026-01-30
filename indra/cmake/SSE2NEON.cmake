# -*- cmake -*-
include_guard()
add_library(ll::sse2neon INTERFACE IMPORTED)

if(DARWIN)
    find_path(SSE2NEON_INCLUDE_DIRS "sse2neon/sse2neon.h" REQUIRED)
    target_include_directories(ll::sse2neon SYSTEM INTERFACE ${SSE2NEON_INCLUDE_DIRS})
endif()
