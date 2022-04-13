# -*- cmake -*-
include(Prebuilt)

include_guard()
create_target( ll::glod )

use_prebuilt_binary(glod)

set(GLODLIB ON CACHE BOOL "Using GLOD library")

set_target_include_dirs( ll::glod ${LIBS_PREBUILT_DIR}/include)
set_target_libraries( ll::glod GLOD )
