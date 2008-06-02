# -*- cmake -*-

include(Linking)
set(JPEG_FIND_QUIETLY ON)
set(JPEG_FIND_REQUIRED ON)

if (STANDALONE)
  include(FindJPEG)
else (STANDALONE)
  if (LINUX)
    set(JPEG_LIBRARIES jpeg)
  elseif (DARWIN)
    set(JPEG_LIBRARIES
      lljpeg
      optimized ${ARCH_PREBUILT_DIRS_RELEASE}/lljpeg
      debug ${ARCH_PREBUILT_DIRS_DEBUG}/lljpeg
      )
  elseif (WINDOWS)
    set(JPEG_LIBRARIES jpeglib_6b)
  endif (LINUX)
  set(JPEG_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include)
endif (STANDALONE)
