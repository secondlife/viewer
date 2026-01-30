# -*- cmake -*-
include_guard()
add_library(ll::vhacd INTERFACE IMPORTED)

find_path(V_HACD_INCLUDE_DIRS "VHACD.h")
target_include_directories(ll::vhacd SYSTEM INTERFACE ${V_HACD_INCLUDE_DIRS})
