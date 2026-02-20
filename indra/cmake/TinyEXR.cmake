# -*- cmake -*-
include_guard()
add_library(ll::tinyexr INTERFACE IMPORTED)

find_package(tinyexr CONFIG REQUIRED)
target_link_libraries(ll::tinyexr INTERFACE unofficial::tinyexr::tinyexr)
