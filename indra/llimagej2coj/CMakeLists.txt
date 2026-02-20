# -*- cmake -*-

add_library(llimagej2coj STATIC)

target_sources(llimagej2coj
    PRIVATE
    llimagej2coj.cpp

    PUBLIC
    FILE_SET HEADERS
    FILES
    llimagej2coj.h
    )

target_link_libraries(llimagej2coj
        PUBLIC
        llimage
        llcommon
        ll::openjpeg
    )

target_precompile_headers(llimagej2coj REUSE_FROM llprecompiled)
