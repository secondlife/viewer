# -*- cmake -*-
include_guard()

add_library(ll::xxhash INTERFACE IMPORTED)

find_package(xxHash CONFIG REQUIRED)
target_link_libraries(ll::xxhash INTERFACE xxHash::xxhash)
