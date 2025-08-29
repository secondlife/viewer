include(Linking)
include(Prebuilt)

include_guard()
add_library( ll::nghttp2 INTERFACE IMPORTED )

use_system_binary(nghttp2)
use_prebuilt_binary(nghttp2)

find_library(NGHTTP2_LIBRARY
    NAMES
    nghttp2.lib
    libnghttp2.a
    PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

target_link_libraries(ll::nghttp2 INTERFACE ${NGHTTP2_LIBRARY})
target_include_directories( ll::nghttp2 SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include/nghttp2)
