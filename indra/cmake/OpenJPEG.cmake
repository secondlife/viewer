# -*- cmake -*-
include(Prebuilt)

include_guard()
create_target( ll::openjpeg )

use_prebuilt_binary(openjpeg)

target_link_libraries(ll::openjpeg INTERFACE openjpeg )
set_target_include_dirs( ll::openjpeg ${LIBS_PREBUILT_DIR}/include/openjpeg)
