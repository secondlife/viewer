# -*- cmake -*-
include_guard()
add_library(ll::openjpeg INTERFACE IMPORTED)

find_package(OpenJPEG CONFIG REQUIRED)
target_link_libraries(ll::openjpeg INTERFACE openjp2)
