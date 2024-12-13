# -*- cmake -*-
include(Prebuilt)

set(NVAPI ON CACHE BOOL "Use NVAPI.")

if (NVAPI)
  if (WINDOWS)
    add_library( ll::nvapi INTERFACE IMPORTED )
    target_link_libraries( ll::nvapi INTERFACE ${ARCH_PREBUILT_DIRS_RELEASE}/nvapi.lib)
    use_prebuilt_binary(nvapi)
  endif (WINDOWS)
endif (NVAPI)

