# -*- cmake -*-

include(Variables)
include(GLEXT)
include(Prebuilt)

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
    set (SDL_FOUND TRUE)
    set (SDL_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/i686-linux)
    set (SDL_LIBRARY SDL directfb fusion direct X11)
  endif (LINUX)
endif (USESYSTEMLIBS)

if (SDL_FOUND)
  include_directories(${SDL_INCLUDE_DIR})
endif (SDL_FOUND)



