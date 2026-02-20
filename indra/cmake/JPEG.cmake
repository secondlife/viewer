# -*- cmake -*-
include_guard()
add_library(ll::libjpeg INTERFACE IMPORTED)

find_package(JPEG REQUIRED)
target_link_libraries(ll::libjpeg INTERFACE JPEG::JPEG)
