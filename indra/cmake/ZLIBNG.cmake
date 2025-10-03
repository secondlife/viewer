# -*- cmake -*-

include(Prebuilt)

include_guard()
add_library( ll::zlib-ng INTERFACE IMPORTED )

if(USE_CONAN )
  target_link_libraries( ll::zlib-ng INTERFACE CONAN_PKG::zlib )
  return()
endif()

use_prebuilt_binary(zlib-ng)
if (WINDOWS)
  target_link_libraries( ll::zlib-ng INTERFACE ${ARCH_PREBUILT_DIRS_RELEASE}/zlib.lib )
else()
  target_link_libraries( ll::zlib-ng INTERFACE ${ARCH_PREBUILT_DIRS_RELEASE}/libz.a )
endif (WINDOWS)

if( NOT LINUX )
  target_include_directories( ll::zlib-ng SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include/zlib-ng)
endif()
