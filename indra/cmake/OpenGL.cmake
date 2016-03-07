# -*- cmake -*-

include(Variables)
include(Prebuilt)

if (BUILD_HEADLESS)
  SET(OPENGL_glu_LIBRARY GLU)
  SET(OPENGL_HEADLESS_LIBRARIES OSMesa16 dl GLU)
endif (BUILD_HEADLESS)

include(FindOpenGL)

