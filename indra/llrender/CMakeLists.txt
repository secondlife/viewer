# -*- cmake -*-

project(llrender)

include(00-Common)
include(FreeType)
include(LLCommon)
include(LLImage)
include(LLMath)
include(LLRender)
include(LLWindow)

include_directories(
    ${FREETYPE_INCLUDE_DIRS}
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLIMAGE_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
    ${LLRENDER_INCLUDE_DIRS}
    ${LLWINDOW_INCLUDE_DIRS}
    )

set(llrender_SOURCE_FILES
    llfont.cpp
    llfontgl.cpp
    llgldbg.cpp
    llglimmediate.cpp
    llimagegl.cpp
    llrendertarget.cpp
    llvertexbuffer.cpp
    llvertexprogramgl.cpp
    )
    
set(llrender_HEADER_FILES
    CMakeLists.txt

    llfontgl.h
    llfont.h
    llgldbg.h
    llglimmediate.h
    llimagegl.h
    llrendertarget.h
    llvertexbuffer.h
    llvertexprogramgl.h
    )

set_source_files_properties(${llrender_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND llrender_SOURCE_FILES ${llrender_HEADER_FILES})

add_library (llrender ${llrender_SOURCE_FILES})
