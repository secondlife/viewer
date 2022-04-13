# -*- cmake -*-
include(Prebuilt)

include_guard()
create_target(ll::libpng)

use_prebuilt_binary(libpng)
if (WINDOWS)
  set_target_libraries(ll::libpng libpng16)
else()
  set_target_libraries(ll::libpng png16 )
endif()
set_target_include_dirs( ll::libpng ${LIBS_PREBUILT_DIR}/include/libpng16)
