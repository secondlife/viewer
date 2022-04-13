# -*- cmake -*-

include(Prebuilt)

include_guard()
create_target(ll::zlib-ng)

use_prebuilt_binary(zlib-ng)
if (WINDOWS)
  set_target_libraries( ll::zlib-ng zlib )
else()
  set_target_libraries( ll::zlib-ng z )
endif (WINDOWS)
set_target_include_dirs( ll::zlib-ng ${LIBS_PREBUILT_DIR}/include/zlib-ng)
