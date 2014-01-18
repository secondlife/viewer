# -*- cmake -*-
include(Prebuilt)

if (NOT STANDALONE)
  use_prebuilt_binary(GLOD)
endif (NOT STANDALONE)

set(GLOD_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/include)
set(GLOD_LIBRARIES GLOD)
