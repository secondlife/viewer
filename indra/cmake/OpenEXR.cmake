# -*- cmake -*-

include(Prebuilt)

include_guard()
add_library( ll::openexr INTERFACE IMPORTED )

if(USE_CONAN )
  target_link_libraries( ll::openexr INTERFACE CONAN_PKG::openexr )
  return()
endif()

use_prebuilt_binary(openexr)
if (WINDOWS)
  target_link_libraries( ll::openexr INTERFACE openexr )
else()
  
endif (WINDOWS)

target_include_directories( ll::openexr SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include/openexr)

