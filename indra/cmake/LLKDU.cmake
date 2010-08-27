# -*- cmake -*-
include(Prebuilt)

if (INSTALL_PROPRIETARY AND NOT STANDALONE)
  use_prebuilt_binary(kdu)
  set(LLKDU_LIBRARY llkdu)
endif (INSTALL_PROPRIETARY AND NOT STANDALONE)
