# -*- cmake -*-
include(Prebuilt)

if (NOT USESYSTEMLIBS)
  if (LINUX OR WINDOWS)
    use_prebuilt_binary(glext)
  endif (LINUX OR WINDOWS)  
  use_prebuilt_binary(glh-linear)
  set(GLEXT_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/include)
endif (NOT USESYSTEMLIBS)
