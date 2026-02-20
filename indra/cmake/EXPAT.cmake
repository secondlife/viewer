# -*- cmake -*-
include_guard()
add_library(ll::expat INTERFACE IMPORTED)

find_package(expat CONFIG REQUIRED)
target_link_libraries(ll::expat INTERFACE expat::expat)
