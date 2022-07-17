# -*- cmake -*-

include_guard()

include(Prebuilt)

add_library( ll::uriparser INTERFACE IMPORTED )

use_system_binary( uriparser )

use_prebuilt_binary(uriparser)
if (WINDOWS)
  target_link_libraries( ll::uriparser INTERFACE uriparser)
elseif (LINUX)
  target_link_libraries( ll::uriparser INTERFACE uriparser)
elseif (DARWIN)
  target_link_libraries( ll::uriparser INTERFACE liburiparser.dylib)
endif (WINDOWS)
target_include_directories( ll::uriparser SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include/uriparser)
