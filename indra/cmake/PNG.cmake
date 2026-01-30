# -*- cmake -*-
include_guard()
add_library(ll::libpng INTERFACE IMPORTED)

find_package(PNG REQUIRED)
target_link_libraries(ll::libpng INTERFACE PNG::PNG)
