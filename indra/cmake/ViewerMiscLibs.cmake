# -*- cmake -*-
include(Prebuilt)

if (LINUX)
  add_library( ll::fontconfig INTERFACE IMPORTED )

  find_package(Fontconfig REQUIRED)
  target_link_libraries( ll::fontconfig INTERFACE  Fontconfig::Fontconfig )
endif (LINUX)

if(NOT USE_VCPKG)
  use_prebuilt_binary(viewer-fonts)
  use_prebuilt_binary(slvoice)
  use_prebuilt_binary(emoji_shortcodes)
endif()
