# -*- cmake -*-
include_guard()
add_library(ll::ndof INTERFACE IMPORTED)

if (USE_NDOF)
  target_compile_definitions(ll::ndof INTERFACE LIB_NDOF=1)

  if(USE_VCPKG AND (WINDOWS OR DARWIN))
    find_library(NDOF_LIBRARY
      NAMES
      libndofdev
      ndofdev
      REQUIRED)
    target_link_libraries(ll::ndof INTERFACE ${NDOF_LIBRARY})
    return()
  endif()

  find_library(NDOF_LIBRARY
      NAMES
      libndofdev
      ndofdev
      PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

  if (LINUX)
    include(SDL3)
    target_link_libraries(ll::ndof INTERFACE ${NDOF_LIBRARY} ll::SDL3)
  else()
    target_link_libraries(ll::ndof INTERFACE ${NDOF_LIBRARY})
  endif()
endif (USE_NDOF)


