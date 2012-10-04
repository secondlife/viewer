# -*- cmake -*-

include(GLEXT)
include(Prebuilt)

if (STANDALONE)
  include(FindSDL)

  # This should be done by FindSDL.  Sigh.
  mark_as_advanced(
      SDLMAIN_LIBRARY
      SDL_INCLUDE_DIR
      SDL_LIBRARY
      )
else (STANDALONE)
  use_prebuilt_binary(mesa)
  if (LINUX)
    use_prebuilt_binary(SDL)
    set (SDL_FOUND TRUE)
    set (SDL_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/i686-linux)
    set (SDL_LIBRARY SDL directfb fusion direct)
  endif (LINUX)
endif (STANDALONE)

if (SDL_FOUND AND NOT BAKING)
  add_definitions(-DLL_SDL=1)
  include_directories(${SDL_INCLUDE_DIR})
endif (SDL_FOUND AND NOT BAKING)

if (BAKING)
  use_prebuilt_binary(mesa)
  add_definitions(-DLL_MESA_HEADLESS=1)
endif (BAKING)

set(LLWINDOW_INCLUDE_DIRS
    ${GLEXT_INCLUDE_DIR}
    ${LIBS_OPEN_DIR}/llwindow
    )

if (BAKING AND LINUX)
  set(LLWINDOW_LIBRARIES
      llwindowheadless
      )
  MESSAGE( STATUS "using headless libraries")
else (BAKING AND LINUX)
  set(LLWINDOW_LIBRARIES
      llwindow
      )
endif (BAKING AND LINUX)
