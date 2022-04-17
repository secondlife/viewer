# -*- cmake -*-

include(Prebuilt)

include_guard()
create_target(ll::zlib-ng)

use_prebuilt_binary(zlib-ng)
if (WINDOWS)
  target_link_libraries( ll::zlib-ng INTERFACE zlib )
else()
  target_link_libraries( ll::zlib-ng INTERFACE z )
endif (WINDOWS)
target_include_directories( ll::zlib-ng SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include/zlib-ng)
