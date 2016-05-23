# -*- cmake -*-
include(Prebuilt)

set(GLH_FIND_REQUIRED TRUE)
set(GLH_FIND_QUIETLY TRUE)

if (USESYSTEMLIBS)
  include(FindGLH)
else (USESYSTEMLIBS)
  use_prebuilt_binary(glh-linear)
endif (USESYSTEMLIBS)
