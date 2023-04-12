# -*- cmake -*-

include_guard()

include(Prebuilt)

add_library( ll::assimp-vc143-mt INTERFACE IMPORTED )

use_system_binary( assimp-vc143-mt )

use_prebuilt_binary(assimp-vc143-mt)
if (WINDOWS)
  target_link_libraries( ll::assimp-vc143-mt INTERFACE assimp-vc143-mt)
elseif (LINUX)
  target_link_libraries( ll::assimp-vc143-mt INTERFACE assimp-vc143-mt)
elseif (DARWIN)
  target_link_libraries( ll::assimp-vc143-mt INTERFACE libassimp-vc143-mt.dylib)
endif (WINDOWS)
target_include_directories( ll::assimp-vc143-mt SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include/assimp-vc143-mt)
