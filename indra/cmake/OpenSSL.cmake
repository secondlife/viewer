# -*- cmake -*-
include(Prebuilt)

set(OpenSSL_FIND_QUIETLY ON)
set(OpenSSL_FIND_REQUIRED ON)

if (USESYSTEMLIBS)
  include(FindOpenSSL)
else (USESYSTEMLIBS)
  use_prebuilt_binary(openssl)
  if (WINDOWS)
    set(OPENSSL_LIBRARIES libssl libcrypto)
  else (WINDOWS)
    set(OPENSSL_LIBRARIES ssl crypto)
  endif (WINDOWS)
  set(OPENSSL_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include)
endif (USESYSTEMLIBS)

if (LINUX)
  set(CRYPTO_LIBRARIES crypto dl)
elseif (DARWIN)
  set(CRYPTO_LIBRARIES crypto)
endif (LINUX)
