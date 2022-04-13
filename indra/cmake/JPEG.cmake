# -*- cmake -*-
include(Prebuilt)

include(Linking)

include_guard()
create_target(ll::jpeglib)

use_prebuilt_binary(jpeglib)
if (LINUX)
  set_target_libraries( ll::jpeglib jpeg)
elseif (DARWIN)
  set_target_libraries( ll::jpeglib jpeg)
elseif (WINDOWS)
  set_target_libraries( ll::jpeglib jpeglib)
endif (LINUX)
set_target_include_dirs( ll::jpeglib ${LIBS_PREBUILT_DIR}/include)
