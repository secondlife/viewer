# -*- cmake -*-
include(Prebuilt)

set(PNG_FIND_QUIETLY ON)
set(PNG_FIND_REQUIRED ON)

if (STANDALONE)
  include(FindPNG)
else (STANDALONE)
  use_prebuilt_binary(libpng)
  if (WINDOWS)
    set(PNG_LIBRARIES 
      debug libpngd
      optimized libpng)
  else (WINDOWS)
    set(PNG_LIBRARIES png12)
  endif (WINDOWS)
  set(PNG_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include)
endif (STANDALONE)
