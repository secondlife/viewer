# -*- cmake -*-
include(Prebuilt)

set(NDOF ON CACHE BOOL "Use NDOF space navigator joystick library.")

if ( TARGET ndof::ndof )
  return()
endif()
create_target( ndof::ndof )

if (NDOF)
  if (USESYSTEMLIBS)
    set(NDOF_FIND_REQUIRED ON)
    include(FindNDOF)
  else (USESYSTEMLIBS)
    if (WINDOWS OR DARWIN)
      use_prebuilt_binary(libndofdev)
    elseif (LINUX)
      use_prebuilt_binary(open-libndofdev)
    endif (WINDOWS OR DARWIN)

    if (WINDOWS)
      set_target_libraries( ndof::ndof libndofdev)
    elseif (DARWIN OR LINUX)
      set_target_libraries( ndof::ndof ndofdev)
    endif (WINDOWS)
    target_compile_definitions( ndof::ndof INTERFACE LIB_NDOF=1)
  endif (USESYSTEMLIBS)
endif (NDOF)

if (NOT NDOF_FOUND)
  message(STATUS "Building without N-DoF joystick support")
endif ()

