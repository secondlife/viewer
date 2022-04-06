# -*- cmake -*-

# these should be moved to their own cmake file
include(Prebuilt)
include(Boost)

if( TARGET colladadom::colladadom )
    return()
endif()

use_prebuilt_binary(colladadom)
use_prebuilt_binary(minizip-ng) # needed for colladadom
use_prebuilt_binary(pcre)
use_prebuilt_binary(libxml2)

create_target( pcre::pcre )
set_target_libraries( pcre::pcre pcrecpp pcre )

create_target( minizip-ng::minizip-ng )
if (WINDOWS)
    set_target_libraries( minizip-ng::minizip-ng libminizip )
else()
    set_target_libraries( minizip-ng::minizip-ng minizip )
endif()

create_target( libxml::libxml )
if (WINDOWS)
    set_target_libraries( libxml::libxml libxml2_a)
else()
    set_target_libraries( libxml::libxml xml2)
endif()

create_target( colladadom::colladadom )
set_target_include_dirs( colladadom::colladadom
        ${LIBS_PREBUILT_DIR}/include/collada
        ${LIBS_PREBUILT_DIR}/include/collada/1.4
        )
if (WINDOWS)
    set_target_libraries(colladadom::colladadom libcollada14dom23-s libxml::libxml minizip-ng::minizip-ng )
elseif (DARWIN)
    set_target_libraries(colladadom::colladadom collada14dom libxml::libxml minizip-ng::minizip-ng)
elseif (LINUX)
    set_target_libraries(colladadom::colladadom collada14dom libxml::libxml minizip-ng::minizip-ng)
endif()