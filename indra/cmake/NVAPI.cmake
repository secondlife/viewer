# -*- cmake -*-
include(Prebuilt)

set(NVAPI ON CACHE BOOL "Use NVAPI.")

if (NVAPI)
  if (WINDOWS)
    create_target( nvapi::nvapi )
    set_target_libraries( nvapi::nvapi nvapi)
    use_prebuilt_binary(nvapi)
  else (WINDOWS)
    set(NVAPI_LIBRARY "")
  endif (WINDOWS)
else (NVAPI)
  set(NVAPI_LIBRARY "")
endif (NVAPI)

