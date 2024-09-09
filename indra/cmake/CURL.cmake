# -*- cmake -*-
include(Prebuilt)
include(Linking)

include_guard()
add_library( ll::libcurl INTERFACE IMPORTED )

use_system_binary(libcurl)
use_prebuilt_binary(curl)
if (WINDOWS)
  target_link_libraries(ll::libcurl INTERFACE
    ${ARCH_PREBUILT_DIRS_RELEASE}/libcurl.lib
    ll::openssl
    ll::nghttp2
    ll::zlib-ng
    )
else ()
  target_link_libraries(ll::libcurl INTERFACE
    ${ARCH_PREBUILT_DIRS_RELEASE}/libcurl.a
    ll::openssl
    ll::nghttp2
    ll::zlib-ng
    )
endif ()
target_include_directories( ll::libcurl SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include)
