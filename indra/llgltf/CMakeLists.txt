# -*- cmake -*-

project(llgltf)

include(00-Common)
include(LLCommon)
include(LLMath)
include(LLPhysicsExtensions)
include(GLM)

set(llgltf_SOURCE_FILES
    gltfprimitive/accessor.cpp
    gltfprimitive/animation.cpp
    gltfprimitive/asset.cpp
    gltfprimitive/primitive.cpp
    gltfprimitive/llgltfloader.cpp
    llgltfmaterialcore.cpp
    llmodel.cpp
    llmodelloader.cpp
    )

set(llgltf_HEADER_FILES
    CMakeLists.txt

    gltfprimitive/accessor.h
    gltfprimitive/animation.h
    gltfprimitive/asset.h
    gltfprimitive/common.h
    gltfprimitive/primitive.h
    gltfprimitive/llgltfloader.h
    gltfprimitive/buffer_util.h
    llgltfmaterialcore.h
    llmodel.h
    llmodelloader.h
    )

list(APPEND llgltf_SOURCE_FILES ${llgltf_HEADER_FILES})

add_library(llgltf ${llgltf_SOURCE_FILES})

target_include_directories(llgltf PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${TINYGLTF_INCLUDE_DIR}
    ${GLM_INCLUDE_DIR}
    )

target_link_libraries(llgltf
    llcommon
    llmath
    llappearance
    llrender
    )

if (HAVOK OR HAVOK_TPV)
    target_link_libraries(llgltf
        llphysicsextensions_impl
        )
else()
    target_link_libraries(llgltf
        llphysicsextensionsos
        )
endif ()

if (USE_PRECOMPILED_HEADERS)
    target_precompile_headers(llgltf REUSE_FROM llprecompiled)
endif ()

#add unit tests
if (LL_TESTS)
    INCLUDE(LLAddBuildTest)
    SET(llgltf_TEST_SOURCE_FILES
      llgltfmaterialcore.cpp
      )

    LL_ADD_PROJECT_UNIT_TESTS(llgltf "${llgltf_TEST_SOURCE_FILES}")
endif (LL_TESTS)
