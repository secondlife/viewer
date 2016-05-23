# -*- cmake -*-
include(Prebuilt)

if (NOT USESYSTEMLIBS)
  use_prebuilt_binary(tut)
endif(NOT USESYSTEMLIBS)
