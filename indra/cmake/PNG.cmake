# -*- cmake -*-
include(Prebuilt)

set(PNG_FIND_QUIETLY ON)
set(PNG_FIND_REQUIRED ON)

if (STANDALONE)
  include(FindPNG)
else (STANDALONE)
  use_prebuilt_binary(libpng)
  set(PNG_LIBRARIES libpng15)
  set(PNG_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include/libpng15)
endif (STANDALONE)
