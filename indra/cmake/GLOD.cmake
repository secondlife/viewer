# -*- cmake -*-
include(Prebuilt)

include_guard()
create_target( ll::glod )

use_prebuilt_binary(glod)

set_target_include_dirs( ll::glod ${LIBS_PREBUILT_DIR}/include)
set_target_libraries( ll::glod GLOD )
target_compile_definitions( ll::glod INTERFACE LL_GLOD=1)