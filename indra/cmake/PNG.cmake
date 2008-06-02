# -*- cmake -*-

set(PNG_FIND_QUIETLY ON)
set(PNG_FIND_REQUIRED ON)

if (STANDALONE)
  include(FindPNG)
else (STANDALONE)
  set(PNG_LIBRARIES png12)
  set(PNG_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include)
endif (STANDALONE)
