# -*- cmake -*-

set(ZLIB_FIND_QUIETLY ON)
set(ZLIB_FIND_REQUIRED ON)

if (STANDALONE)
  include(FindZLIB)
else (STANDALONE)
  if (WINDOWS)
    set(ZLIB_LIBRARIES zlib)
  else (WINDOWS)
    set(ZLIB_LIBRARIES z)
  endif (WINDOWS)
  set(ZLIB_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include)
endif (STANDALONE)
