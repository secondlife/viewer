# -*- cmake -*-
include(Prebuilt)

include_guard()
create_target( ll::xmlrpc-epi )

use_prebuilt_binary(xmlrpc-epi)
target_link_libraries(ll::xmlrpc-epi INTERFACE xmlrpc-epi )
set_target_include_dirs( ll::xmlrpc-epi ${LIBS_PREBUILT_DIR}/include)
