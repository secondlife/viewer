# -*- cmake -*-

include(Variables)
include(GLEXT)
include(Prebuilt)
include(SDL3)

include_guard()

if (LINUX)
  set(USE_SDL_WINDOW ON CACHE BOOL "Build with SDL window backend")
else()
  set(USE_SDL_WINDOW OFF CACHE BOOL "Build with SDL window backend")
endif (LINUX)
