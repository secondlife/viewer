# -*- cmake -*-
include(Prebuilt)

if( TARGET openssl::openssl )
  return()
endif()
create_target(openssl::openssl)

if (USESYSTEMLIBS)
  include(FindOpenSSL)
else (USESYSTEMLIBS)
  use_prebuilt_binary(openssl)
  if (WINDOWS)
    set_target_libraries(openssl::openssl libssl libcrypto)
  elseif (LINUX)
    set_target_libraries(openssl::openssl ssl crypto dl)
  else()
    set_target_libraries(openssl::openssl ssl crypto)
  endif (WINDOWS)
  set_target_include_dirs(openssl::openssl ${LIBS_PREBUILT_DIR}/include)
endif (USESYSTEMLIBS)

