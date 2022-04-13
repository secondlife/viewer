# -*- cmake -*-
include(Prebuilt)

set(NVAPI ON CACHE BOOL "Use NVAPI.")

if (NVAPI)
  if (WINDOWS)
    create_target( ll::nvapi )
    set_target_libraries( ll::nvapi nvapi)
    use_prebuilt_binary(nvapi)
  endif (WINDOWS)
endif (NVAPI)

