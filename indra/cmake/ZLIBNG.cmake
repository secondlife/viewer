# -*- cmake -*-

include_guard()
add_library(ll::zlib-ng INTERFACE IMPORTED)

find_package(ZLIB REQUIRED)
target_link_libraries(ll::zlib-ng INTERFACE ZLIB::ZLIB)
