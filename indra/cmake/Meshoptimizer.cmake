# -*- cmake -*-
include_guard()
add_library(ll::meshoptimizer INTERFACE IMPORTED)

find_package(meshoptimizer CONFIG REQUIRED)
target_link_libraries(ll::meshoptimizer INTERFACE meshoptimizer::meshoptimizer)
