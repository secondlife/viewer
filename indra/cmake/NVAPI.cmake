# -*- cmake -*-
include_guard()

if (USE_NVAPI)
  if (WINDOWS)
    add_library( ll::nvapi INTERFACE IMPORTED )
    find_library(NVAPI_LIBRARY nvapi64 REQUIRED)
    target_link_libraries(ll::nvapi INTERFACE ${NVAPI_LIBRARY})
  endif (WINDOWS)
endif (USE_NVAPI)

