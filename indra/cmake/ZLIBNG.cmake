# -*- cmake -*-

include(Prebuilt)

if( TARGET zlib-ng::zlib-ng )
  return()
endif()
create_target(zlib-ng::zlib-ng)

use_prebuilt_binary(zlib-ng)
if (WINDOWS)
  set_target_libraries( zlib-ng::zlib-ng zlib )
else()
  set_target_libraries( zlib-ng::zlib-ng z )
endif (WINDOWS)
set_target_include_dirs( zlib-ng::zlib-ng ${LIBS_PREBUILT_DIR}/include/zlib-ng)
