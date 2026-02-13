# -*- cmake -*-
include_guard()
add_library(ll::meshoptimizer INTERFACE IMPORTED)

if(USE_VCPKG)
    find_package(meshoptimizer CONFIG REQUIRED)
    target_link_libraries(ll::meshoptimizer INTERFACE meshoptimizer::meshoptimizer)
    return()
endif()

include(Linking)
include(Prebuilt)

use_system_binary(meshoptimizer)
use_prebuilt_binary(meshoptimizer)

find_library(MESHOPTIMIZER_LIBRARY
    NAMES
    meshoptimizer.lib
    libmeshoptimizer.a
    PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

target_link_libraries(ll::meshoptimizer INTERFACE ${MESHOPTIMIZER_LIBRARY})

target_include_directories(ll::meshoptimizer SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include/meshoptimizer)
