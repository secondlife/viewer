# -*- cmake -*-

include(Variables)
include(GLEXT)
include(Prebuilt)

if( TARGET sdl::sdl)
  return()
endif()
create_target(sdl::sdl)

if (USESYSTEMLIBS)
  include(FindSDL)

  # This should be done by FindSDL.  Sigh.
  mark_as_advanced(
      SDLMAIN_LIBRARY
      SDL_INCLUDE_DIR
      SDL_LIBRARY
      )
else (USESYSTEMLIBS)
  if (LINUX)
    use_prebuilt_binary(SDL)
    set_target_include_dirs( sdl::sdl ${LIBS_PREBUILT_DIR}/i686-linux)
    set_target_libraries( sdl::sdl SDL directfb fusion direct X11)
    target_compile_definitions( sdl::sdl INTERFACE LL_SDL=1)
  endif (LINUX)
endif (USESYSTEMLIBS)


