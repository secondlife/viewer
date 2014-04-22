# -*- cmake -*-

set(ZLIB_FIND_QUIETLY ON)
set(ZLIB_FIND_REQUIRED ON)

include(Prebuilt)

if (USESYSTEMLIBS)
  include(FindZLIB)
else (USESYSTEMLIBS)
  use_prebuilt_binary(zlib)
  if (WINDOWS)
    set(ZLIB_LIBRARIES 
      debug zlibd
      optimized zlib)
  else (WINDOWS)
    set(ZLIB_LIBRARIES z)
  endif (WINDOWS)
  if (WINDOWS OR LINUX)
    set(ZLIB_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include/zlib)
  endif (WINDOWS OR LINUX)
endif (USESYSTEMLIBS)
