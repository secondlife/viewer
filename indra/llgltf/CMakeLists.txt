# -*- cmake -*-

project(llgltf)

include(00-Common)
include(LLCommon)
include(LLMath)
include(LLPrimitive)
include(GLM)

set(llgltf_SOURCE_FILES
    gltfprimitive/accessor.cpp
    gltfprimitive/animation.cpp
    gltfprimitive/asset.cpp
    gltfprimitive/primitive.cpp
    gltfprimitive/llgltfloader.cpp
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
    llprimitive
    )

if (USE_PRECOMPILED_HEADERS)
    target_precompile_headers(llgltf REUSE_FROM llprecompiled)
endif ()
