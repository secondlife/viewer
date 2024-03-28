# -*- cmake -*-

include(Prebuilt)

include_guard()
add_library( ll::openexr INTERFACE IMPORTED )

if(USE_CONAN )
  target_link_libraries( ll::openexr INTERFACE CONAN_PKG::openexr )
  return()
endif()

use_prebuilt_binary(openexr)

target_link_libraries( ll::openexr INTERFACE Iex-3_2 IlmThread-3_2 Imath-3_1 OpenEXR-3_2 OpenEXRCore-3_2 OpenEXRUtil-3_2)

target_include_directories( ll::openexr SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include/OpenEXR ${LIBS_PREBUILT_DIR}/include/Imath)

