# -*- cmake -*-
include_guard()
add_library(ll::nanosvg INTERFACE IMPORTED)

if(USE_VCPKG)
    find_package(NanoSVG CONFIG REQUIRED)
    target_link_libraries(ll::nanosvg INTERFACE NanoSVG::nanosvg NanoSVG::nanosvgrast)
    return()
endif()

include(Prebuilt)

use_system_binary(nanosvg)
use_prebuilt_binary(nanosvg)

target_include_directories(ll::nanosvg SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include)
