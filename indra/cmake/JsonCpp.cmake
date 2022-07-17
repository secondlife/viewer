# -*- cmake -*-

include(Prebuilt)
include_guard()
add_library( ll::jsoncpp INTERFACE IMPORTED )

use_system_binary(jsoncpp)

use_prebuilt_binary(jsoncpp)
if (WINDOWS)
  target_link_libraries( ll::jsoncpp INTERFACE json_libmd.lib )
elseif (DARWIN)
  target_link_libraries( ll::jsoncpp INTERFACE libjson_darwin_libmt.a )
elseif (LINUX)
  target_link_libraries( ll::jsoncpp INTERFACE libjson_linux-gcc-4.1.3_libmt.a )
endif (WINDOWS)
target_include_directories( ll::jsoncpp SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include)
