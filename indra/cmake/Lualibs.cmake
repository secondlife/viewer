# -*- cmake -*-

include_guard()

include(Prebuilt)

add_library( ll::lualibs INTERFACE IMPORTED )

use_system_binary( lualibs )

use_prebuilt_binary(lualibs)

target_link_libraries(ll::lualibs INTERFACE ${lualibs})

target_include_directories( ll::lualibs SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include/lualibs)
