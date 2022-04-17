# -*- cmake -*-
include(Prebuilt)

include_guard()
add_library( ll::glod INTERFACE IMPORTED )

use_prebuilt_binary(glod)

target_include_directories( ll::glod SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include)
target_link_libraries( ll::glod INTERFACE GLOD )
target_compile_definitions( ll::glod INTERFACE LL_GLOD=1)