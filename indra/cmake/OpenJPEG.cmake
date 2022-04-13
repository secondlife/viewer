# -*- cmake -*-
include(Prebuilt)

include_guard()
create_target( ll::openjpeg )

use_prebuilt_binary(openjpeg)

set_target_libraries(ll::openjpeg openjpeg )
set_target_include_dirs( ll::openjpeg ${LIBS_PREBUILT_DIR}/include/openjpeg)
