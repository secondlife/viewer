# -*- cmake -*-

include_guard()

include(Prebuilt)

add_library( ll::assimp INTERFACE IMPORTED )

use_system_binary( assimp )

use_prebuilt_binary(assimp)
if (WINDOWS)
  target_link_libraries( ll::assimp INTERFACE assimp-vc143-mt)
elseif (LINUX)
  target_link_libraries( ll::assimp INTERFACE assimp)
elseif (DARWIN)
  target_link_libraries( ll::assimp INTERFACE libassimp.dylib)
endif (WINDOWS)
target_include_directories( ll::assimp SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include/assimp)
