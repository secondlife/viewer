# -*- cmake -*-
include(Prebuilt)

include_guard()
add_library( ll::expat INTERFACE IMPORTED )

use_system_binary(expat)
use_prebuilt_binary(expat)
if (WINDOWS)
    target_link_libraries( ll::expat  INTERFACE libexpatMT )
else (WINDOWS)
    target_link_libraries( ll::expat INTERFACE expat )
endif (WINDOWS)
target_include_directories( ll::expat SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include )
