# -*- cmake -*-
include(Prebuilt)

include_guard()
create_target(ll::libcurl)

use_prebuilt_binary(curl)
if (WINDOWS)
  set_target_libraries(ll::libcurl libcurl.lib)
else (WINDOWS)
  set_target_libraries(ll::libcurl libcurl.a)
endif (WINDOWS)
set_target_include_dirs( ll::libcurl ${LIBS_PREBUILT_DIR}/include)
