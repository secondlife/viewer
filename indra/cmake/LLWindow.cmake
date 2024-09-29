# -*- cmake -*-

include(Variables)
include(GLEXT)
include(Prebuilt)

include_guard()
add_library( ll::SDL INTERFACE IMPORTED )

if (LINUX)
  #Must come first as use_system_binary can exit this file early
  target_compile_definitions( ll::SDL INTERFACE LL_SDL_VERSION=2 LL_SDL)

  use_prebuilt_binary(SDL2)
  target_link_libraries( ll::SDL INTERFACE SDL2 X11)
endif (LINUX)
