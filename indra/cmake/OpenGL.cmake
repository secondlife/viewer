# -*- cmake -*-
include(Prebuilt)

if (NOT STANDALONE)
  use_prebuilt_binary(GL)
  # possible glh_linear should have its own .cmake file instead
  use_prebuilt_binary(glh_linear)
  set(GLEXT_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/include)
endif (NOT STANDALONE)
