# -*- cmake -*-
include_guard()
add_library(ll::glm INTERFACE IMPORTED)

if(USE_VCPKG)
    find_package(glm CONFIG REQUIRED)
    target_link_libraries(ll::glm INTERFACE glm::glm-header-only)
    return()
endif()

include(Prebuilt)

use_system_binary(glm)
use_prebuilt_binary(glm)
