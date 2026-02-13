# -*- cmake -*-
include_guard()
include(Prebuilt)

add_library( ll::glext INTERFACE IMPORTED )
if(USE_VCPKG)
    find_path(OPENGL_REGISTRY_INCLUDE_DIRS "GL/glcorearb.h")
    target_include_directories(ll::glext SYSTEM INTERFACE ${OPENGL_REGISTRY_INCLUDE_DIRS})
    return()
endif()

use_system_binary(glext)
use_prebuilt_binary(glext)


