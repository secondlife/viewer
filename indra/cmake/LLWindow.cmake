# -*- cmake -*-

include(Variables)
include(GLEXT)
include(Prebuilt)

include_guard()
add_library( ll::sdl INTERFACE IMPORTED )

if (LINUX)
  use_prebuilt_binary(SDL)
  target_include_directories( ll::sdl SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/i686-linux)
  target_link_libraries( ll::sdl INTERFACE SDL directfb fusion direct X11)
  target_compile_definitions( ll::sdl INTERFACE LL_SDL=1)
endif (LINUX)


