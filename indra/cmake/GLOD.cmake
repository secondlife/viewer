# -*- cmake -*-
include(Prebuilt)

if (NOT USESYSTEMLIBS)
  use_prebuilt_binary(glod)
endif (NOT USESYSTEMLIBS)

set(GLOD_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/include)
set(GLOD_LIBRARIES GLOD)
