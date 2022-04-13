# -*- cmake -*-
include(Prebuilt)

if( TARGET libcurl::libcurl )
  return()
endif()
create_target(libcurl::libcurl)

use_prebuilt_binary(curl)
if (WINDOWS)
  set_target_libraries(libcurl::libcurl libcurl.lib)
else (WINDOWS)
  set_target_libraries(libcurl::libcurl libcurl.a)
endif (WINDOWS)
set_target_include_dirs( libcurl::libcurl ${LIBS_PREBUILT_DIR}/include)
