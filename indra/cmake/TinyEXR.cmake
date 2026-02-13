# -*- cmake -*-
include_guard()
add_library(ll::tinyexr INTERFACE IMPORTED)

if(USE_VCPKG)
    find_package(tinyexr CONFIG REQUIRED)
    target_link_libraries(ll::tinyexr INTERFACE unofficial::tinyexr::tinyexr)
    return()
endif()

include(Prebuilt)

use_prebuilt_binary(tinyexr)

target_include_directories(ll::tinyexr SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include/tinyexr)
