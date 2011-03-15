# -*- cmake -*-
include(Prebuilt)

set(OpenSSL_FIND_QUIETLY ON)
set(OpenSSL_FIND_REQUIRED ON)

if (STANDALONE)
  include(FindOpenSSL)
else (STANDALONE)
  use_prebuilt_binary(openSSL)
  if (WINDOWS)
    set(OPENSSL_LIBRARIES ssleay32 libeay32)
  else (WINDOWS)
    set(OPENSSL_LIBRARIES ssl)
  endif (WINDOWS)
  set(OPENSSL_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include)
endif (STANDALONE)

if (LINUX)
  set(CRYPTO_LIBRARIES crypto)
elseif (DARWIN)
  set(CRYPTO_LIBRARIES crypto)
endif (LINUX)
