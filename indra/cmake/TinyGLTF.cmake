# -*- cmake -*-
include_guard()
add_library( ll::tinygltf INTERFACE IMPORTED )

find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h" REQUIRED)
target_include_directories(ll::tinygltf SYSTEM INTERFACE ${TINYGLTF_INCLUDE_DIRS})
