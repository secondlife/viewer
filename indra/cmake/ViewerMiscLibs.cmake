# -*- cmake -*-
include(Prebuilt)

if (LINUX)
  #use_prebuilt_binary(libuuid)
  add_library( ll::fontconfig INTERFACE IMPORTED )

  find_package(Fontconfig REQUIRED)
  target_link_libraries( ll::fontconfig INTERFACE  Fontconfig::Fontconfig )
endif (LINUX)

if( NOT USE_CONAN )
  use_prebuilt_binary(libhunspell)
endif()

use_prebuilt_binary(slvoice)

