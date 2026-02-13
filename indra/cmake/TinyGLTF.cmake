# -*- cmake -*-
include_guard()
add_library( ll::tinygltf INTERFACE IMPORTED )

if(USE_VCPKG)
    find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")
target_include_directories(ll::tinygltf SYSTEM INTERFACE ${TINYGLTF_INCLUDE_DIRS})
    return()
endif()

include(Prebuilt)

use_prebuilt_binary(tinygltf)

target_include_directories(ll::tinygltf SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include/tinygltf)

