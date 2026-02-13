# -*- cmake -*-
include_guard()
add_library(ll::vhacd INTERFACE IMPORTED)

if(USE_VCPKG)
  find_path(V_HACD_INCLUDE_DIRS "VHACD.h")
target_include_directories(ll::vhacd SYSTEM INTERFACE ${V_HACD_INCLUDE_DIRS})

  return()
endif()
include(Prebuilt)

use_system_binary(vhacd)
use_prebuilt_binary(vhacd)

target_include_directories(ll::vhacd SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include/vhacd/)
