# -*- cmake -*-

include(Variables)
include(Prebuilt)

if (BUILD_HEADLESS)
  use_prebuilt_binary(mesa)
  SET(OPENGL_glu_LIBRARY GLU)
  SET(OPENGL_HEADLESS_LIBRARIES OSMesa16 GLU)
endif (BUILD_HEADLESS)

include(FindOpenGL)

