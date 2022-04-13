# -*- cmake -*-
include(Prebuilt)

set(NDOF ON CACHE BOOL "Use NDOF space navigator joystick library.")

include_guard()
create_target( ll::ndof )

if (NDOF)
  if (WINDOWS OR DARWIN)
    use_prebuilt_binary(libndofdev)
  elseif (LINUX)
    use_prebuilt_binary(open-libndofdev)
  endif (WINDOWS OR DARWIN)

  if (WINDOWS)
    set_target_libraries( ll::ndof libndofdev)
  elseif (DARWIN OR LINUX)
    set_target_libraries( ll::ndof ndofdev)
  endif (WINDOWS)
  target_compile_definitions( ll::ndof INTERFACE LIB_NDOF=1)
endif (NDOF)

if (NOT NDOF_FOUND)
  message(STATUS "Building without N-DoF joystick support")
endif ()

