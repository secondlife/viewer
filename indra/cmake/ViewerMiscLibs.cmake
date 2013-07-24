# -*- cmake -*-
include(Prebuilt)

if (NOT STANDALONE)
  use_prebuilt_binary(libhunspell)
  use_prebuilt_binary(libuuid)
  use_prebuilt_binary(slvoice)
  use_prebuilt_binary(fontconfig)
endif(NOT STANDALONE)

