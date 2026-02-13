# -*- cmake -*-
include_guard()
add_library( ll::libcurl INTERFACE IMPORTED )

if(USE_VCPKG)
    find_package(CURL REQUIRED)
    target_link_libraries(ll::libcurl INTERFACE CURL::libcurl)
    return()
endif()

include(Prebuilt)
include(Linking)
include(ZLIBNG)
include(OpenSSL)
include(NGHTTP2)

use_system_binary(libcurl)
use_prebuilt_binary(curl)

find_library(CURL_LIBRARY
    NAMES
    libcurl.lib
    libcurl.a
    PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

target_link_libraries(ll::libcurl INTERFACE ${CURL_LIBRARY} ll::openssl ll::nghttp2 ll::zlib-ng)

target_include_directories(ll::libcurl SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include)

target_compile_definitions(ll:libcurl INTERFACE CURL_STATICLIB=1)
