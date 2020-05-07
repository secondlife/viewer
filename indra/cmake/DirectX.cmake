# -*- cmake -*-

if (WINDOWS)
  find_path(DIRECTX_INCLUDE_DIR dxdiag.h)
  if (DIRECTX_INCLUDE_DIR)
    if (NOT DIRECTX_FIND_QUIETLY)
      message(STATUS "Found DirectX include: ${DIRECTX_INCLUDE_DIR}")
    endif (NOT DIRECTX_FIND_QUIETLY)
  else (DIRECTX_INCLUDE_DIR)
    message(FATAL_ERROR "Could not find DirectX SDK Include")
  endif (DIRECTX_INCLUDE_DIR)

  # dxhint isn't meant to be the hard-coded DIRECTX_LIBRARY_DIR, we're just
  # suggesting it as a hint to the next find_path(). The version is embedded
  # in the DIRECTX_INCLUDE_DIR path string after Include and Lib, which is why
  # we don't just append a relative path: if there are multiple versions
  # installed on the host, we need to be sure we're using THE SAME version.
  string(REPLACE "/Include/" "/Lib/" dxhint "${DIRECTX_INCLUDE_DIR}")
  if (ADDRESS_SIZE EQUAL 32)
    set(archdir x86)
  else()
    set(archdir x64)
  endif()
  string(APPEND dxhint "/${archdir}")
  find_path(DIRECTX_LIBRARY_DIR dxguid.lib HINTS "${dxhint}")
  if (DIRECTX_LIBRARY_DIR)
    if (NOT DIRECTX_FIND_QUIETLY)
      message(STATUS "Found DirectX library: ${DIRECTX_LIBRARY_DIR}")
    endif (NOT DIRECTX_FIND_QUIETLY)
  else (DIRECTX_LIBRARY_DIR)
    message(FATAL_ERROR "Could not find DirectX SDK Libraries")
  endif (DIRECTX_LIBRARY_DIR)

endif (WINDOWS)
