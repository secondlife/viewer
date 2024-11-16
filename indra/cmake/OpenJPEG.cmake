# -*- cmake -*-
include(Prebuilt)

include_guard()
add_library( ll::openjpeg INTERFACE IMPORTED )

use_system_binary(openjpeg)
use_prebuilt_binary(openjpeg)

if(DARWIN)
target_link_libraries(ll::openjpeg INTERFACE ${ARCH_PREBUILT_DIRS_RELEASE}/libopenjp2.a )
else()
target_link_libraries(ll::openjpeg INTERFACE openjp2 )
endif()
target_include_directories( ll::openjpeg SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include/openjpeg)
