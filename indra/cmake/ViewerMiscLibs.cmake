# -*- cmake -*-
include(Prebuilt)

if (NOT STANDALONE)
  use_prebuilt_binary(libstdc++)
  use_prebuilt_binary(libuuid)
  if(INSTALL_PROPRIETARY)
    use_prebuilt_binary(vivox)
  endif(INSTALL_PROPRIETARY)
endif(NOT STANDALONE)

