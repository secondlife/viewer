# -*- cmake -*-

include(Variables)
include(GLEXT)
include(Prebuilt)

include_guard()
create_target(ll::sdl)

if (LINUX)
  use_prebuilt_binary(SDL)
  set_target_include_dirs( ll::sdl ${LIBS_PREBUILT_DIR}/i686-linux)
  target_link_libraries( ll::sdl INTERFACE SDL directfb fusion direct X11)
  target_compile_definitions( ll::sdl INTERFACE LL_SDL=1)
endif (LINUX)


