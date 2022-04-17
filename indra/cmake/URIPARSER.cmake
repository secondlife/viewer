# -*- cmake -*-

include_guard()
create_target( ll::uriparser )

include(Prebuilt)

use_prebuilt_binary(uriparser)
if (WINDOWS)
  target_link_libraries( ll::uriparser INTERFACE uriparser)
elseif (LINUX)
  target_link_libraries( ll::uriparser INTERFACE uriparser)
elseif (DARWIN)
  target_link_libraries( ll::uriparser INTERFACE liburiparser.dylib)
endif (WINDOWS)
set_target_include_dirs( ll::uriparser ${LIBS_PREBUILT_DIR}/include/uriparser)
