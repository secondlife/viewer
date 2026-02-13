# -*- cmake -*-
include_guard()

if (USE_NVAPI)
  if (WINDOWS)
    add_library( ll::nvapi INTERFACE IMPORTED )
    if(USE_VCPKG)
      find_library(NVAPI_LIBRARY nvapi64 REQUIRED)
      target_link_libraries(ll::nvapi INTERFACE ${NVAPI_LIBRARY})
      return()
    endif()

    include(Prebuilt)
    use_prebuilt_binary(nvapi)
    target_link_libraries( ll::nvapi INTERFACE nvapi)
  endif (WINDOWS)
endif (USE_NVAPI)

