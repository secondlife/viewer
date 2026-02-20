# -*- cmake -*-
include_guard()
add_library(ll::nanosvg INTERFACE IMPORTED)

find_package(NanoSVG CONFIG REQUIRED)
target_link_libraries(ll::nanosvg INTERFACE NanoSVG::nanosvg NanoSVG::nanosvgrast)
