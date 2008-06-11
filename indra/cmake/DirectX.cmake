# -*- cmake -*-

if (VIEWER AND WINDOWS)
  find_path(DIRECTX_INCLUDE_DIR dxdiag.h
            "$ENV{DXSDK_DIR}/Include"
            "C:/Program Files/Microsoft DirectX SDK (March 2008)/Include"
            "C:/Program Files/Microsoft DirectX SDK (November 2007)/Include"
            "C:/Program Files/Microsoft DirectX SDK (August 2007)/Include"
            "C:/DX90SDK/Include"
            "C:/Program Files/DX90SDK/Include"
            )
  if (DIRECTX_INCLUDE_DIR)
    include_directories(${DIRECTX_INCLUDE_DIR})
    if (DIRECTX_FIND_QUIETLY)
      message(STATUS "Found DirectX include: ${DIRECTX_INCLUDE_DIR}")
    endif (DIRECTX_FIND_QUIETLY)
  else (DIRECTX_INCLUDE_DIR)
    message(FATAL_ERROR "Could not find DirectX SDK Include")
  endif (DIRECTX_INCLUDE_DIR)


  find_path(DIRECTX_LIBRARY_DIR dxguid.lib
            "$ENV{DXSDK_DIR}/Lib/x86"
            "C:/Program Files/Microsoft DirectX SDK (March 2008)/Lib/x86"
            "C:/Program Files/Microsoft DirectX SDK (November 2007)/Lib/x86"
            "C:/Program Files/Microsoft DirectX SDK (August 2007)/Lib/x86"
            "C:/DX90SDK/Lib"
            "C:/Program Files/DX90SDK/Lib"
            )
  if (DIRECTX_LIBRARY_DIR)
    if (DIRECTX_FIND_QUIETLY)
      message(STATUS "Found DirectX include: ${DIRECTX_LIBRARY_DIR}")
    endif (DIRECTX_FIND_QUIETLY)
  else (DIRECTX_LIBRARY_DIR)
    message(FATAL_ERROR "Could not find DirectX SDK Libraries")
  endif (DIRECTX_LIBRARY_DIR)

endif (VIEWER AND WINDOWS)
