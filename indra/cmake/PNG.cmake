# -*- cmake -*-
include(Prebuilt)

include_guard()
create_target(ll::libpng)

use_prebuilt_binary(libpng)
if (WINDOWS)
  target_link_libraries(ll::libpng INTERFACE libpng16)
else()
  target_link_libraries(ll::libpng INTERFACE png16 )
endif()
target_include_directories( ll::libpng SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include/libpng16)
