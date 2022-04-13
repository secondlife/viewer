# -*- cmake -*-

include(Prebuilt)
include_guard()
create_target( ll::jsoncpp)

use_prebuilt_binary(jsoncpp)
if (WINDOWS)
  set_target_libraries( ll::jsoncpp json_libmd.lib )
elseif (DARWIN)
  set_target_libraries( ll::jsoncpp libjson_darwin_libmt.a )
elseif (LINUX)
  set_target_libraries( ll::jsoncpp libjson_linux-gcc-4.1.3_libmt.a )
endif (WINDOWS)
set_target_include_dirs( ll::jsoncpp ${LIBS_PREBUILT_DIR}/include/json)
