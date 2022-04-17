# -*- cmake -*-
include(Prebuilt)

include_guard()
create_target( ll::glod )

use_prebuilt_binary(glod)

target_include_directories( ll::glod SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include)
target_link_libraries( ll::glod INTERFACE GLOD )
target_compile_definitions( ll::glod INTERFACE LL_GLOD=1)