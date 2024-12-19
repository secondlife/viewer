# -*- cmake -*-
include(Prebuilt)

set(NDOF ON CACHE BOOL "Use NDOF space navigator joystick library.")

include_guard()
add_library( ll::ndof INTERFACE IMPORTED )

if (NDOF)
  if (WINDOWS OR DARWIN)
    use_prebuilt_binary(libndofdev)
  elseif (LINUX)
    use_prebuilt_binary(open-libndofdev)
  endif (WINDOWS OR DARWIN)

  if (WINDOWS)
    target_link_libraries( ll::ndof INTERFACE ${ARCH_PREBUILT_DIRS_RELEASE}/libndofdev.lib)
  elseif (DARWIN)
    target_link_libraries( ll::ndof INTERFACE ${ARCH_PREBUILT_DIRS_RELEASE}/libndofdev.dylib)
  elseif (LINUX)
    target_link_libraries( ll::ndof INTERFACE ${ARCH_PREBUILT_DIRS_RELEASE}/libndofdev.a)
  endif (WINDOWS)
  target_compile_definitions( ll::ndof INTERFACE LIB_NDOF=1)
else()
  add_compile_options(-ULIB_NDOF)
endif (NDOF)
