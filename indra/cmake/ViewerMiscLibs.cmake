# -*- cmake -*-
include(Prebuilt)

if (LINUX)
  use_prebuilt_binary(libuuid)
  use_prebuilt_binary(fontconfig)
  add_library( ll::fontconfig INTERFACE IMPORTED )
endif (LINUX)
use_prebuilt_binary(libhunspell)
use_prebuilt_binary(slvoice)

