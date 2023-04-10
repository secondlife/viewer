# -*- cmake -*-
include(Prebuilt)

include(Linking)

include_guard()
add_library( ll::libjpeg INTERFACE IMPORTED )

use_system_binary(libjpeg)
use_prebuilt_binary(jpeglib)
if (LINUX)
  target_link_libraries( ll::libjpeg INTERFACE jpeg)
elseif (DARWIN)
  target_link_libraries( ll::libjpeg INTERFACE jpeg)
elseif (WINDOWS)
  target_link_libraries( ll::libjpeg INTERFACE jpeglib)
endif (LINUX)
target_include_directories( ll::libjpeg SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include)
