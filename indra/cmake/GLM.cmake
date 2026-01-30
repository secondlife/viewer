# -*- cmake -*-
include_guard()
add_library(ll::glm INTERFACE IMPORTED)

find_package(glm CONFIG REQUIRED)
target_link_libraries(ll::glm INTERFACE glm::glm-header-only)
