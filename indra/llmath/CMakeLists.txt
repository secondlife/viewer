# -*- cmake -*-

project(llmath)

include(00-Common)
include(LLCommon)

include_directories(
    ${LLCOMMON_INCLUDE_DIRS}
    )

set(llmath_SOURCE_FILES
    llbboxlocal.cpp
    llcamera.cpp
    llcoordframe.cpp
    llline.cpp
    llperlin.cpp
    llquaternion.cpp
    llrect.cpp
    llsphere.cpp
    llvolume.cpp
    llvolumemgr.cpp
    llsdutil_math.cpp
    m3math.cpp
    m4math.cpp
    raytrace.cpp
    v2math.cpp
    v3color.cpp
    v3dmath.cpp
    v3math.cpp
    v4color.cpp
    v4coloru.cpp
    v4math.cpp
    xform.cpp
    )

set(llmath_HEADER_FILES
    CMakeLists.txt

    camera.h
    coordframe.h
    llbboxlocal.h
    llcamera.h
    llcoord.h
    llcoordframe.h
    llinterp.h
    llline.h
    llmath.h
    lloctree.h
    llperlin.h
    llplane.h
    llquantize.h
    llquaternion.h
    llrect.h
    llsphere.h
    lltreenode.h
    llv4math.h
    llv4matrix3.h
    llv4matrix4.h
    llv4vector3.h
    llvolume.h
    llvolumemgr.h
    llsdutil_math.h
    m3math.h
    m4math.h
    raytrace.h
    v2math.h
    v3color.h
    v3dmath.h
    v3math.h
    v4color.h
    v4coloru.h
    v4math.h
    xform.h
    )

set_source_files_properties(${llmath_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND llmath_SOURCE_FILES ${llmath_HEADER_FILES})

add_library (llmath ${llmath_SOURCE_FILES})
