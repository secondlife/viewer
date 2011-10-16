# -*- cmake -*-
include(Prebuilt)

set(GLH_FIND_REQUIRED TRUE)
set(GLH_FIND_QUIETLY TRUE)

if (STANDALONE)
  include(FindGLH)
else (STANDALONE)
  use_prebuilt_binary(glh_linear)
endif (STANDALONE)
