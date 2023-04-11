# -*- cmake -*-
include(Prebuilt)

if (LINUX)
  #use_prebuilt_binary(libuuid)
  add_library( ll::fontconfig INTERFACE IMPORTED )

  if( NOT USE_CONAN )
    use_prebuilt_binary(fontconfig)
  else()
    target_link_libraries( ll::fontconfig INTERFACE CONAN_PKG::fontconfig )
  endif()
endif (LINUX)

if( NOT USE_CONAN )
  use_prebuilt_binary(libhunspell)
endif()

use_prebuilt_binary(slvoice)

use_prebuilt_binary(nanosvg)
use_prebuilt_binary(viewer-fonts)
use_prebuilt_binary(emoji_shortcodes)
