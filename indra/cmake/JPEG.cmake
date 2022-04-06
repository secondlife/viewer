# -*- cmake -*-
include(Prebuilt)

include(Linking)

if( TARGET jpeglib::jpeglib )
  return()
endif()
create_target(jpeglib::jpeglib)

if (USESYSTEMLIBS)
  include(FindJPEG)
else (USESYSTEMLIBS)
  use_prebuilt_binary(jpeglib)
  if (LINUX)
    set_target_libraries( jpeglib::jpeglib jpeg)
  elseif (DARWIN)
    set_target_libraries( jpeglib::jpeglib jpeg)
  elseif (WINDOWS)
    set_target_libraries( jpeglib::jpeglib jpeglib)
  endif (LINUX)
  set_target_include_dirs( jpeglib::jpeglib ${LIBS_PREBUILT_DIR}/include)
endif (USESYSTEMLIBS)
