# -*- cmake -*-
include(Prebuilt)

include_guard()
create_target(ll::openssl)

use_prebuilt_binary(openssl)
if (WINDOWS)
  target_link_libraries(ll::openssl INTERFACE libssl libcrypto)
elseif (LINUX)
  target_link_libraries(ll::openssl INTERFACE ssl crypto dl)
else()
  target_link_libraries(ll::openssl INTERFACE ssl crypto)
endif (WINDOWS)
target_include_directories( ll::openssl SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include)

