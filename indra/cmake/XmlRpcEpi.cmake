# -*- cmake -*-
include(Prebuilt)

include_guard()
add_library( ll::xmlrpc-epi INTERFACE IMPORTED )

use_system_binary( xmlrpc-epi )

use_prebuilt_binary(xmlrpc-epi)
target_link_libraries(ll::xmlrpc-epi INTERFACE xmlrpc-epi )
target_include_directories( ll::xmlrpc-epi SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include)
