# -*- cmake -*-

include(Variables)
include(GLEXT)
include(Prebuilt)
include(SDL2)

include_guard()

if (LINUX)
  # linux uses SDL2 for window and keyboard
  target_compile_definitions( ll::SDL2 INTERFACE LL_USE_SDL_WINDOW=1 LL_USE_SDL_KEYBOARD=1 )
endif (LINUX)
