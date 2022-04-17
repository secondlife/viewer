# -*- cmake -*-
include(Prebuilt)

include_guard()
create_target(ll::libcurl)

use_prebuilt_binary(curl)
if (WINDOWS)
  target_link_libraries(ll::libcurl INTERFACE libcurl.lib)
else (WINDOWS)
  target_link_libraries(ll::libcurl INTERFACE libcurl.a)
endif (WINDOWS)
set_target_include_dirs( ll::libcurl ${LIBS_PREBUILT_DIR}/include)
