# -*- cmake -*-

include(Prebuilt)

include_guard()
add_library( ll::zlib-ng INTERFACE IMPORTED )

if(USE_CONAN )
  target_link_libraries( ll::zlib-ng INTERFACE CONAN_PKG::zlib )
  return()
endif()

use_prebuilt_binary(zlib-ng)

find_library(ZLIBNG_LIBRARY
    NAMES
    zlib.lib
    libz.a
    PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

target_link_libraries(ll::zlib-ng INTERFACE ${ZLIBNG_LIBRARY})

if( NOT LINUX )
  target_include_directories( ll::zlib-ng SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include/zlib-ng)
endif()
