# -*- cmake -*-
if (LINUX)
  add_library( ll::fontconfig INTERFACE IMPORTED )

  find_package(Fontconfig REQUIRED)
  target_link_libraries( ll::fontconfig INTERFACE  Fontconfig::Fontconfig )
endif (LINUX)

