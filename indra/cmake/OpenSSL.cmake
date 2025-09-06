# -*- cmake -*-
include(Prebuilt)
include(Linking)

include_guard()
add_library( ll::openssl INTERFACE IMPORTED )

use_system_binary(openssl)
use_prebuilt_binary(openssl)

find_library(SSL_LIBRARY
    NAMES
    libssl.lib
    libssl.a
    PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

find_library(CRYPTO_LIBRARY
    NAMES
    libcrypto.lib
    libcrypto.a
    PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

target_link_libraries(ll::openssl INTERFACE ${SSL_LIBRARY} ${CRYPTO_LIBRARY})

if (WINDOWS)
  target_link_libraries(ll::openssl INTERFACE Crypt32.lib)
endif (WINDOWS)

target_include_directories(ll::openssl SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include)

