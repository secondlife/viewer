# -*- cmake -*-

include(Variables)
include(GLEXT)
include(Prebuilt)

if( TARGET sdl::sdl)
  return()
endif()
create_target(sdl::sdl)

if (LINUX)
  use_prebuilt_binary(SDL)
  set_target_include_dirs( sdl::sdl ${LIBS_PREBUILT_DIR}/i686-linux)
  set_target_libraries( sdl::sdl SDL directfb fusion direct X11)
  target_compile_definitions( sdl::sdl INTERFACE LL_SDL=1)
endif (LINUX)


