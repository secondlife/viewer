# -*- cmake -*-
include(Prebuilt)

include(Linking)
set(JPEG_FIND_QUIETLY ON)
set(JPEG_FIND_REQUIRED ON)

if (STANDALONE)
  include(FindJPEG)
else (STANDALONE)
  use_prebuilt_binary(jpeglib)
  if (LINUX)
    set(JPEG_LIBRARIES jpeg)
  elseif (DARWIN)
    set(JPEG_LIBRARIES
      optimized ${ARCH_PREBUILT_DIRS_RELEASE}/liblljpeg.a
      debug ${ARCH_PREBUILT_DIRS_DEBUG}/liblljpeg.a
      )
  elseif (WINDOWS)
    set(JPEG_LIBRARIES jpeglib_6b)
  endif (LINUX)
  set(JPEG_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include)
endif (STANDALONE)
