# -*- cmake -*-
include(Prebuilt)

if (NOT STANDALONE)
  use_prebuilt_binary(libstdc++)
  use_prebuilt_binary(libuuid)
  use_prebuilt_binary(vivox)
endif(NOT STANDALONE)

