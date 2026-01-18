# -*- cmake -*-
include_guard()
add_library(ll::mikktspace INTERFACE IMPORTED)
target_include_directories(ll::mikktspace SYSTEM INTERFACE ${CMAKE_SOURCE_DIR}/externals/mikktspace/)

