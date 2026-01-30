# -*- cmake -*-
include_guard()
add_library(ll::vorbis INTERFACE IMPORTED)

find_package(Vorbis CONFIG REQUIRED)
target_link_libraries(ll::vorbis INTERFACE Vorbis::vorbisfile Vorbis::vorbisenc Vorbis::vorbis)
