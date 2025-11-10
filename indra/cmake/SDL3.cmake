# -*- cmake -*-
include_guard()
include(Linking)
include(Prebuilt)

add_library(ll::SDL3 INTERFACE IMPORTED)

if (LINUX)
  set(USE_SDL_WINDOW ON CACHE BOOL "Build with SDL window backend")
else()
  set(USE_SDL_WINDOW OFF CACHE BOOL "Build with SDL window backend")
endif (LINUX)

if(USE_SDL_WINDOW)
    use_system_binary(SDL3)
    use_prebuilt_binary(SDL3)

    find_library( SDL3_LIBRARY
        NAMES SDL3 SDL3.lib libSDL3.so libSDL3.dylib
        PATHS "${LIBS_PREBUILT_DIR}/lib/release" REQUIRED)

    target_link_libraries(ll::SDL3 INTERFACE ${SDL3_LIBRARY})
    target_include_directories(ll::SDL3 SYSTEM INTERFACE "${LIBS_PREBUILT_DIR}/include/")

endif(USE_SDL_WINDOW)

