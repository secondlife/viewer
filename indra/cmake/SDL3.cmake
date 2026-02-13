# -*- cmake -*-
include_guard()
add_library(ll::SDL3 INTERFACE IMPORTED)

if(NOT USE_SDL_WINDOW)
    return()
endif()

if(USE_VCPKG)
    find_package(SDL3 CONFIG REQUIRED)
    target_link_libraries(ll::SDL3 INTERFACE SDL3::SDL3)
    return()
endif()

include(Linking)
include(Prebuilt)

use_system_binary(SDL3)
use_prebuilt_binary(SDL3)

find_library( SDL3_LIBRARY
    NAMES SDL3 SDL3.lib libSDL3.so libSDL3.dylib
    PATHS "${LIBS_PREBUILT_DIR}/lib/release" REQUIRED)

target_link_libraries(ll::SDL3 INTERFACE ${SDL3_LIBRARY})
target_include_directories(ll::SDL3 SYSTEM INTERFACE "${LIBS_PREBUILT_DIR}/include/")

