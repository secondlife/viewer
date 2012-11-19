# -*- cmake -*-
include(Prebuilt)

set(NVAPI ON CACHE BOOL "Use NVAPI.")

if (NVAPI)
  use_prebuilt_binary(nvapi)
  
  if (WINDOWS)
    set(NVAPI_LIBRARY nvapi)
  else (WINDOWS)
    set(NVAPI_LIBRARY "")
  endif (WINDOWS)
else (NVAPI)
  set(NVAPI_LIBRARY "")
endif (NVAPI)

