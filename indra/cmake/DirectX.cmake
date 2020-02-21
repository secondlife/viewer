# -*- cmake -*-

if (WINDOWS)
  if(ADDRESS_SIZE EQUAL 32)
    set(PROGRAMFILES_x86 $ENV{PROGRAMFILES})
  else(ADDRESS_SIZE EQUAL 32)
    set(PROGRAMFILES_x86 $ENV{PROGRAMFILES\(X86\)})
  endif(ADDRESS_SIZE EQUAL 32)

  find_path(DIRECTX_INCLUDE_DIR dxdiag.h
            "$ENV{DXSDK_DIR}/Include"
            "${PROGRAMFILES_x86}/Windows Kits/8.1/Include/um"
            "$ENV{PROGRAMFILES}/Microsoft DirectX SDK (June 2010)/Include"
            "$ENV{PROGRAMFILES}/Microsoft DirectX SDK (August 2009)/Include"
            "$ENV{PROGRAMFILES}/Microsoft DirectX SDK (March 2009)/Include"
            "$ENV{PROGRAMFILES}/Microsoft DirectX SDK (August 2008)/Include"
            "$ENV{PROGRAMFILES}/Microsoft DirectX SDK (June 2008)/Include"
            "$ENV{PROGRAMFILES}/Microsoft DirectX SDK (March 2008)/Include"
            "$ENV{PROGRAMFILES}/Microsoft DirectX SDK (November 2007)/Include"
            "$ENV{PROGRAMFILES}/Microsoft DirectX SDK (August 2007)/Include"
            "C:/DX90SDK/Include"
            "$ENV{PROGRAMFILES}/DX90SDK/Include"
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
            "${PROGRAMFILES_x86}/Windows Kits/8.1/Lib/winv6.3/um/x86"
            "$ENV{PROGRAMFILES}/Microsoft DirectX SDK (June 2010)/Lib/x86"
            "$ENV{PROGRAMFILES}/Microsoft DirectX SDK (August 2009)/Lib/x86"
            "$ENV{PROGRAMFILES}/Microsoft DirectX SDK (March 2009)/Lib/x86"
            "$ENV{PROGRAMFILES}/Microsoft DirectX SDK (August 2008)/Lib/x86"
            "$ENV{PROGRAMFILES}/Microsoft DirectX SDK (June 2008)/Lib/x86"
            "$ENV{PROGRAMFILES}/Microsoft DirectX SDK (March 2008)/Lib/x86"
            "$ENV{PROGRAMFILES}/Microsoft DirectX SDK (November 2007)/Lib/x86"
            "$ENV{PROGRAMFILES}/Microsoft DirectX SDK (August 2007)/Lib/x86"
            "C:/DX90SDK/Lib"
            "$ENV{PROGRAMFILES}/DX90SDK/Lib"
            )
  if (DIRECTX_LIBRARY_DIR)
    if (DIRECTX_FIND_QUIETLY)
      message(STATUS "Found DirectX include: ${DIRECTX_LIBRARY_DIR}")
    endif (DIRECTX_FIND_QUIETLY)
  else (DIRECTX_LIBRARY_DIR)
    message(FATAL_ERROR "Could not find DirectX SDK Libraries")
  endif (DIRECTX_LIBRARY_DIR)

endif (WINDOWS)
