# -*- cmake -*-
include_guard()

add_library(ll::xxhash INTERFACE IMPORTED)

if(USE_VCPKG)
    find_package(xxHash CONFIG REQUIRED)
    target_link_libraries(ll::xxhash INTERFACE xxHash::xxhash)
    return()
endif()

include(Prebuilt)
use_prebuilt_binary(xxhash)
