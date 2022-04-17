# -*- cmake -*-

include(Prebuilt)
include_guard()
create_target( ll::jsoncpp)

use_prebuilt_binary(jsoncpp)
if (WINDOWS)
  target_link_libraries( ll::jsoncpp INTERFACE json_libmd.lib )
elseif (DARWIN)
  target_link_libraries( ll::jsoncpp INTERFACE libjson_darwin_libmt.a )
elseif (LINUX)
  target_link_libraries( ll::jsoncpp INTERFACE libjson_linux-gcc-4.1.3_libmt.a )
endif (WINDOWS)
set_target_include_dirs( ll::jsoncpp ${LIBS_PREBUILT_DIR}/include/json)
