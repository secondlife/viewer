# -*- cmake -*-
include_guard()
add_library(ll::openxr INTERFACE IMPORTED)
if(USE_OPENXR)
  find_package(OpenXR CONFIG REQUIRED)
  target_link_libraries(ll::openxr INTERFACE OpenXR::headers OpenXR::openxr_loader)
endif()
