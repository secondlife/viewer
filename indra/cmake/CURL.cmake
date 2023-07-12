# -*- cmake -*-
include(Prebuilt)

include_guard()
add_library( ll::libcurl INTERFACE IMPORTED )

use_system_binary(libcurl)
use_prebuilt_binary(curl)
if (WINDOWS)
  target_link_libraries(ll::libcurl INTERFACE libcurl.lib)
else (WINDOWS)
  target_link_libraries(ll::libcurl INTERFACE libcurl.a)
endif (WINDOWS)
target_include_directories( ll::libcurl SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include)
