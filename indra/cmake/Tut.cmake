# -*- cmake -*-
include_guard()
add_library(ll::tut INTERFACE IMPORTED)
target_include_directories(ll::tut SYSTEM INTERFACE ${INDRA_SOURCE_DIR}/externals/tut/)
