include(Linking)
include(Prebuilt)

include_guard()
add_library( ll::nghttp2 INTERFACE IMPORTED )

use_system_binary(nghttp2)
use_prebuilt_binary(nghttp2)
if (WINDOWS)
  target_link_libraries( ll::nghttp2 INTERFACE ${ARCH_PREBUILT_DIRS_RELEASE}/nghttp2.lib)
else ()
  target_link_libraries( ll::nghttp2 INTERFACE ${ARCH_PREBUILT_DIRS_RELEASE}/libnghttp2.a)
endif ()
target_include_directories( ll::nghttp2 SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include/nghttp2)
