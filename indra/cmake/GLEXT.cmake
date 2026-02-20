# -*- cmake -*-
include_guard()

add_library(ll::glext INTERFACE IMPORTED)

find_path(OPENGL_REGISTRY_INCLUDE_DIRS "GL/glcorearb.h" REQUIRED)
target_include_directories(ll::glext SYSTEM INTERFACE ${OPENGL_REGISTRY_INCLUDE_DIRS})
