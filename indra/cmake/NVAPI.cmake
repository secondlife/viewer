# -*- cmake -*-
include_guard()

add_library(ll::nvapi INTERFACE IMPORTED)

if (USE_NVAPI)
  if (WINDOWS)
    find_library(NVAPI_LIBRARY nvapi64 REQUIRED)
    target_link_libraries(ll::nvapi INTERFACE ${NVAPI_LIBRARY})
  endif (WINDOWS)
endif (USE_NVAPI)

