# -*- cmake -*-
include(Prebuilt)

if( TARGET xmlrpc-epi::xmlrpc-epi )
    return()
endif()
create_target( xmlrpc-epi::xmlrpc-epi )

use_prebuilt_binary(xmlrpc-epi)
set_target_libraries(xmlrpc-epi::xmlrpc-epi xmlrpc-epi )
set_target_include_dirs( xmlrpc-epi::xmlrpc-epi ${LIBS_PREBUILT_DIR}/include)
