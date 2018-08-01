# -*- cmake -*-

project(llimage)

include(00-Common)
include(LLCommon)
include(LLImage)
include(LLMath)
include(LLVFS)
include(LLKDU)
include(OpenJPEG)
include(LLImageJ2COJ)
include(ZLIB)
include(LLAddBuildTest)
include(Tut)

include_directories(
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLCOMMON_SYSTEM_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
    ${LLVFS_INCLUDE_DIRS}
    ${PNG_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIRS}
    )

set(llimage_SOURCE_FILES
    llimagebmp.cpp
    llimage.cpp
    llimagedimensionsinfo.cpp
    llimagedxt.cpp
    llimagefilter.cpp
    llimagej2c.cpp
    llimagejpeg.cpp
    llimagepng.cpp
    llimagetga.cpp
    llpngwrapper.cpp
    )

set(llimage_HEADER_FILES
    CMakeLists.txt

    llimage.h
    llimagebmp.h
    llimagedimensionsinfo.h
    llimagedxt.h
    llimagefilter.h
    llimagej2c.h
    llimagejpeg.h
    llimagepng.h
    llimagetga.h
    llmapimagetype.h
    llpngwrapper.h
    )

set_source_files_properties(${llimage_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND llimage_SOURCE_FILES ${llimage_HEADER_FILES})

add_library (llimage ${llimage_SOURCE_FILES})
# Libraries on which this library depends, needed for Linux builds
# Sort by high-level to low-level
if (USE_KDU)
    target_link_libraries(llimage ${LLKDU_LIBRARIES})
    add_definitions(-DUSE_KDU=1)
endif (USE_KDU)

if (USE_OPENJPEG)
    target_link_libraries(llimage ${LLIMAGEJ2COJ_LIBRARIES})
    add_definitions(-DUSE_OPENJPEG=1)
endif (USE_OPENJPEG)

target_link_libraries(llimage
    ${LLVFS_LIBRARIES}
    ${LLMATH_LIBRARIES}
    ${LLCOMMON_LIBRARIES}
    ${JPEG_LIBRARIES}
    ${PNG_LIBRARIES}
    ${ZLIB_LIBRARIES}
    )
