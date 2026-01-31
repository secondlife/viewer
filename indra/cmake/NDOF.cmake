# -*- cmake -*-
include_guard()
add_library(ll::ndof INTERFACE IMPORTED)

if (USE_NDOF)
  target_compile_definitions(ll::ndof INTERFACE LIB_NDOF=1)

  find_library(NDOF_LIBRARY
    NAMES
    libndofdev
    ndofdev
    REQUIRED)
  target_link_libraries(ll::ndof INTERFACE ${NDOF_LIBRARY})

  if (LINUX)
    include(SDL3)
    target_link_libraries(ll::ndof INTERFACE ll::SDL3)
  endif()
endif (USE_NDOF)


