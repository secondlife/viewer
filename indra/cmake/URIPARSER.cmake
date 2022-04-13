# -*- cmake -*-

include_guard()
create_target( ll::uriparser )

include(Prebuilt)

use_prebuilt_binary(uriparser)
if (WINDOWS)
  set_target_libraries( ll::uriparser uriparser)
elseif (LINUX)
  set_target_libraries( ll::uriparser uriparser)
elseif (DARWIN)
  set_target_libraries( ll::uriparser liburiparser.dylib)
endif (WINDOWS)
set_target_include_dirs( ll::uriparser ${LIBS_PREBUILT_DIR}/include/uriparser)
