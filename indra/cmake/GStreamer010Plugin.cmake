# -*- cmake -*-
include(Prebuilt)
if (NOT LINUX)
  return()
endif()

add_library( ll::gstreamer INTERFACE IMPORTED )
target_compile_definitions( ll::gstreamer INTERFACE LL_GSTREAMER010_ENABLED=1)
use_system_binary(gstreamer)

use_prebuilt_binary(gstreamer)

