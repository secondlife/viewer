# -*- cmake -*-
include_guard()
add_library(ll::freetype INTERFACE IMPORTED)

find_package(Freetype REQUIRED)
target_link_libraries(ll::freetype INTERFACE Freetype::Freetype)
