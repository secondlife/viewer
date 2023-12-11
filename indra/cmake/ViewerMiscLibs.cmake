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

if (INSTALL_PROPRIETARY)
  set(USE_SLVOICE ON CACHE BOOL "Using slvoice for voice feature.")
endif ()

if( USE_SLVOICE )
  use_prebuilt_binary(slvoice)
endif()
