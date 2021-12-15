# -*- cmake -*-
use_prebuilt_binary(threejs)

# Main three.js file
configure_file("${AUTOBUILD_INSTALL_DIR}/js/three.min.js" "${CMAKE_SOURCE_DIR}/newview/skins/default/html/common/equirectangular/js/three.min.js" COPYONLY)

# Controls to move around the scene using mouse or keyboard
configure_file("${AUTOBUILD_INSTALL_DIR}/js/OrbitControls.js" "${CMAKE_SOURCE_DIR}/newview/skins/default/html/common/equirectangular/js/OrbitControls.js" COPYONLY)
