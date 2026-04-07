# -*- cmake -*-

project(llmath)

include(00-Common)
include(LLCommon)
include(bugsplat)
include(Boost)
include(SSE2NEON)

set(llmath_SOURCE_FILES
    llbbox.cpp
    llbboxlocal.cpp
    llcalc.cpp
    llcalcparser.cpp
    llcamera.cpp
    llcoordframe.cpp
    llline.cpp
    llmatrix3a.cpp
    llmatrix4a.cpp
    llmodularmath.cpp
    lloctree.cpp
    llperlin.cpp
    llquaternion.cpp
    llrigginginfo.cpp
    llrect.cpp
    llsphere.cpp
    llvector4a.cpp
    llvolume.cpp
    llvolumemgr.cpp
    llvolumeoctree.cpp
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
    llcalc.h
    llcalcparser.h
    llcamera.h
    llcoord.h
    llcoordframe.h
    llinterp.h
    llline.h
    llmath.h
    llmatrix3a.h
    llmatrix3a.inl
    llmatrix4a.h
    llmodularmath.h
    lloctree.h
    llperlin.h
    llplane.h
    llquantize.h
    llquaternion.h
    llquaternion2.h
    llquaternion2.inl
    llrect.h
    llrigginginfo.h
    llsimdmath.h
    llsimdtypes.h
    llsimdtypes.inl
    llsphere.h
    lltreenode.h
    llvector4a.h
    llvector4a.inl
    llvector4logical.h
    llvolume.h
    llvolumemgr.h
    llvolumeoctree.h
    llsdutil_math.h
    m3math.h
    m4math.h
    raytrace.h
    v2math.h
    v3color.h
    v3colorutil.h
    v3dmath.h
    v3math.h
    v4color.h
    v4coloru.h
    v4math.h
    xform.h
    )

list(APPEND llmath_SOURCE_FILES ${llmath_HEADER_FILES})

add_library (llmath ${llmath_SOURCE_FILES})

target_link_libraries(llmath llcommon llmeshoptimizer ll::sse2neon)
target_include_directories( llmath  INTERFACE   ${CMAKE_CURRENT_SOURCE_DIR})

# Add tests
if (LL_TESTS)
  include(LLAddBuildTest)
  # UNIT TESTS
  SET(llmath_TEST_SOURCE_FILES
    llbboxlocal.cpp
    llmodularmath.cpp
    llrect.cpp
    v2math.cpp
    v3color.cpp
    v4color.cpp
    v4coloru.cpp
    )

  LL_ADD_PROJECT_UNIT_TESTS(llmath "${llmath_TEST_SOURCE_FILES}")

  # INTEGRATION TESTS
  set(test_libs llmath llcommon)
  # TODO: Some of these need refactoring to be proper Unit tests rather than Integration tests.
  LL_ADD_INTEGRATION_TEST(alignment "" "${test_libs}")
  LL_ADD_INTEGRATION_TEST(llbbox llbbox.cpp "${test_libs}")
  LL_ADD_INTEGRATION_TEST(llquaternion llquaternion.cpp "${test_libs}")
  LL_ADD_INTEGRATION_TEST(mathmisc "" "${test_libs}")
  LL_ADD_INTEGRATION_TEST(m3math "" "${test_libs}")
  LL_ADD_INTEGRATION_TEST(m4math m4math.cpp "${test_libs}")

  # Differential tests that compare LL math types against their glm
  # counterparts for the same operations. Pulls in the glm interface
  # target. The macro doesn't handle namespaced interface targets in
  # its lib list, so wire glm in directly after the executable is
  # created (see test_coverage.md → CMake quirks).
  include(GLM)
  LL_ADD_INTEGRATION_TEST(m4math_glm_equivalence "" "${test_libs}")
  target_link_libraries(INTEGRATION_TEST_m4math_glm_equivalence ll::glm)
  LL_ADD_INTEGRATION_TEST(v3math_glm_equivalence "" "${test_libs}")
  target_link_libraries(INTEGRATION_TEST_v3math_glm_equivalence ll::glm)
  LL_ADD_INTEGRATION_TEST(v4math_glm_equivalence "" "${test_libs}")
  target_link_libraries(INTEGRATION_TEST_v4math_glm_equivalence ll::glm)
  LL_ADD_INTEGRATION_TEST(llquaternion_glm_equivalence llquaternion.cpp "${test_libs}")
  target_link_libraries(INTEGRATION_TEST_llquaternion_glm_equivalence ll::glm)
  LL_ADD_INTEGRATION_TEST(m3math_glm_equivalence "" "${test_libs}")
  target_link_libraries(INTEGRATION_TEST_m3math_glm_equivalence ll::glm)
  LL_ADD_INTEGRATION_TEST(v2math_glm_equivalence "" "${test_libs}")
  target_link_libraries(INTEGRATION_TEST_v2math_glm_equivalence ll::glm)
  LL_ADD_INTEGRATION_TEST(v3color_glm_equivalence "" "${test_libs}")
  target_link_libraries(INTEGRATION_TEST_v3color_glm_equivalence ll::glm)
  LL_ADD_INTEGRATION_TEST(v4color_glm_equivalence "" "${test_libs}")
  target_link_libraries(INTEGRATION_TEST_v4color_glm_equivalence ll::glm)
  LL_ADD_INTEGRATION_TEST(v3dmath v3dmath.cpp "${test_libs}")
  LL_ADD_INTEGRATION_TEST(v3math v3math.cpp "${test_libs}")
  LL_ADD_INTEGRATION_TEST(v4math v4math.cpp "${test_libs}")
  LL_ADD_INTEGRATION_TEST(xform xform.cpp "${test_libs}")

  # GoogleTest tests — infrastructure is in cmake/{GTest,LLAddGTest}.cmake
  # but currently parked. The test exe builds and runs (exit 0), but the
  # llcommon-linked SL test environment hijacks stdout via APR/LL_INFOS,
  # so gtest's printf output is never visible. New tests use TUT instead.
  # See modernization_plan.md → Testing → "gtest stdout hijack" for details.
  # include(LLAddGTest)
  # LL_ADD_GTEST(llmath llmatrix4 "")
endif (LL_TESTS)
