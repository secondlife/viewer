# -*- cmake -*-
include(Prebuilt)

if( TARGET openjpeg::openjpeg )
  return()
endif()
create_target( openjpeg::openjpeg )

use_prebuilt_binary(openjpeg)

set_target_libraries(openjpeg::openjpeg openjpeg )
set_target_include_dirs( openjpeg::openjpeg ${LIBS_PREBUILT_DIR}/include/openjpeg)
