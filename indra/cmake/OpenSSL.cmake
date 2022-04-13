# -*- cmake -*-
include(Prebuilt)

include_guard()
create_target(ll::openssl)

use_prebuilt_binary(openssl)
if (WINDOWS)
  set_target_libraries(ll::openssl libssl libcrypto)
elseif (LINUX)
  set_target_libraries(ll::openssl ssl crypto dl)
else()
  set_target_libraries(ll::openssl ssl crypto)
endif (WINDOWS)
set_target_include_dirs(ll::openssl ${LIBS_PREBUILT_DIR}/include)

