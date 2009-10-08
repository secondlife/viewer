# -*- cmake -*-

project(llmath)

include(00-Common)
include(LLCommon)

include_directories(
    ${LLCOMMON_INCLUDE_DIRS}
    )

set(llmath_SOURCE_FILES
    llbbox.cpp
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
    llbbox.h
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

# Add tests
include(LLAddBuildTest)
SET(llmath_TEST_SOURCE_FILES
  # nat 2009-08-28: found this commented out and considered implementing it
  # using LL_ADD_INTEGRATION_TEST, but there's no llvolume_test.cpp source?
  # llvolume.cpp
  )
LL_ADD_PROJECT_UNIT_TESTS(llmath "${llmath_TEST_SOURCE_FILES}")

set(test_libs llmath llcommon ${LLCOMMON_LIBRARIES} ${WINDOWS_LIBRARIES})
# some of these are probably pretty decent *unit* tests rather than integration tests, but I have to work out a CMake whining puzzle.
LL_ADD_INTEGRATION_TEST(llbbox llbbox.cpp "${test_libs}")
LL_ADD_INTEGRATION_TEST(llbboxlocal llbboxlocal.cpp "${test_libs}")
LL_ADD_INTEGRATION_TEST(mathmisc "" "${test_libs}")
LL_ADD_INTEGRATION_TEST(llrect "llrect.cpp" "${test_libs}")
LL_ADD_INTEGRATION_TEST(llquaternion llquaternion.cpp "${test_libs}")
LL_ADD_INTEGRATION_TEST(v2math v2math.cpp "${test_libs}")
LL_ADD_INTEGRATION_TEST(v3color v3color.cpp "${test_libs}")
LL_ADD_INTEGRATION_TEST(v3dmath v3dmath.cpp "${test_libs}")
LL_ADD_INTEGRATION_TEST(v3math v3math.cpp "${test_libs}")
LL_ADD_INTEGRATION_TEST(v4color v4color.cpp "${test_libs}")
LL_ADD_INTEGRATION_TEST(v4coloru v4coloru.cpp "${test_libs}")
LL_ADD_INTEGRATION_TEST(v4math v4math.cpp "${test_libs}")
LL_ADD_INTEGRATION_TEST(xform xform.cpp "${test_libs}")
