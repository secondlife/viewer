# -*- cmake -*-

# these should be moved to their own cmake file
include(Prebuilt)
include(Boost)

include_guard()

add_library( ll::pcre INTERFACE IMPORTED )
add_library( ll::minizip-ng INTERFACE IMPORTED )
add_library( ll::libxml INTERFACE IMPORTED )
add_library( ll::colladadom INTERFACE IMPORTED )

# ND, needs fixup in collada conan pkg
if( USE_CONAN )
  target_include_directories( ll::colladadom SYSTEM INTERFACE
    "${CONAN_INCLUDE_DIRS_COLLADADOM}/collada-dom/" 
    "${CONAN_INCLUDE_DIRS_COLLADADOM}/collada-dom/1.4/" )
endif()

use_system_binary( colladadom )

use_prebuilt_binary(colladadom)
use_prebuilt_binary(minizip-ng) # needed for colladadom
use_prebuilt_binary(pcre)
use_prebuilt_binary(libxml2)

target_link_libraries( ll::pcre INTERFACE pcrecpp pcre )

if (WINDOWS)
    target_link_libraries( ll::minizip-ng INTERFACE libminizip )
else()
    target_link_libraries( ll::minizip-ng INTERFACE minizip )
endif()

if (WINDOWS)
    target_link_libraries( ll::libxml INTERFACE libxml2_a)
else()
    target_link_libraries( ll::libxml INTERFACE xml2)
endif()

target_include_directories( ll::colladadom SYSTEM INTERFACE
        ${LIBS_PREBUILT_DIR}/include/collada
        ${LIBS_PREBUILT_DIR}/include/collada/1.4
        )
if (WINDOWS)
    target_link_libraries(ll::colladadom INTERFACE libcollada14dom23-s ll::libxml ll::minizip-ng )
elseif (DARWIN)
    target_link_libraries(ll::colladadom INTERFACE collada14dom ll::libxml ll::minizip-ng)
elseif (LINUX)
    target_link_libraries(ll::colladadom INTERFACE collada14dom ll::libxml ll::minizip-ng)
endif()
