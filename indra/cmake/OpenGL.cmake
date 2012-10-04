# -*- cmake -*-

#if (INSTALL_PROPRIETARY)
#  set(HEADLESS ON CACHE BOOL "Use headless mesa library.")
#endif (INSTALL_PROPRIETARY)

include(Prebuilt)

if (LINUX AND NOT STANDALONE)
  use_prebuilt_binary(mesa)
  SET(OPENGL_glu_LIBRARY GLU)
endif (LINUX AND NOT STANDALONE)

include(FindOpenGL)

