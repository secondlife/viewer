# -*- cmake -*-
include(Prebuilt)

include_guard()
create_target( ll::xmlrpc-epi )

use_prebuilt_binary(xmlrpc-epi)
target_link_libraries(ll::xmlrpc-epi INTERFACE xmlrpc-epi )
target_include_directories( ll::xmlrpc-epi SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include)
