# -*- cmake -*-

project(llimagej2coj)

include(00-Common)
include(LLCommon)
include(LLImage)
include(OpenJPEG)

set(llimagej2coj_SOURCE_FILES
    llimagej2coj.cpp
    )

set(llimagej2coj_HEADER_FILES
    CMakeLists.txt

    llimagej2coj.h
    )

list(APPEND llimagej2coj_SOURCE_FILES ${llimagej2coj_HEADER_FILES})

add_library (llimagej2coj ${llimagej2coj_SOURCE_FILES})

target_link_libraries( llimagej2coj
        llcommon
        llimage
        ll::openjpeg
    )

