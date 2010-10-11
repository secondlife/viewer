# -*- cmake -*-
include(Prebuilt)

set(PNG_FIND_QUIETLY ON)
set(PNG_FIND_REQUIRED ON)

if (STANDALONE)
  include(FindPNG)
else (STANDALONE)
  use_prebuilt_binary(libpng)
  set(PNG_LIBRARIES png12)
  set(PNG_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include/libpng12)
endif (STANDALONE)
