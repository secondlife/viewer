# -*- cmake -*-
cmake_minimum_required( VERSION 3.13 FATAL_ERROR )

include(Linking)
include( Prebuilt )
include_guard()

add_library( ll::SDL3 INTERFACE IMPORTED )

use_system_binary( SDL3 )
use_prebuilt_binary( SDL3 )

find_library( SDL3_LIBRARY
    NAMES SDL3 SDL3.lib libSDL3.so libSDL3.dylib
    PATHS "${LIBS_PREBUILT_DIR}/lib/release" REQUIRED)

target_link_libraries( ll::SDL3 INTERFACE ${SDL3_LIBRARY})
target_include_directories( ll::SDL3 SYSTEM INTERFACE "${LIBS_PREBUILT_DIR}/include/" )

