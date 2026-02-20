# -*- cmake -*-
include_guard()
add_library( ll::openssl INTERFACE IMPORTED )

find_package(OpenSSL REQUIRED)
target_link_libraries(ll::openssl INTERFACE OpenSSL::SSL OpenSSL::Crypto)
