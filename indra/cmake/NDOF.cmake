# -*- cmake -*-
include_guard()
add_library(ll::ndof INTERFACE IMPORTED)

if (USE_NDOF)
  target_compile_definitions(ll::ndof INTERFACE LIB_NDOF=1)

  find_library(NDOF_LIBRARY_RELEASE
    NAMES
    libndofdev
    ndofdev
    PATHS "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib"
    REQUIRED
    NO_DEFAULT_PATH
  )

  target_link_libraries(ll::ndof INTERFACE optimized ${NDOF_LIBRARY_RELEASE})

  find_library(NDOF_LIBRARY_DEBUG
    NAMES
    libndofdev
    ndofdev
    PATHS "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/lib"
    NO_DEFAULT_PATH
  )

  if (NOT NDOF_LIBRARY_DEBUG STREQUAL "NDOF_LIBRARY_DEBUG-NOTFOUND")
    target_link_libraries(ll::ndof INTERFACE debug ${NDOF_LIBRARY_DEBUG})
  endif()

  if (LINUX)
    include(SDL3)
    target_link_libraries(ll::ndof INTERFACE ll::SDL3)
  endif()
endif (USE_NDOF)


