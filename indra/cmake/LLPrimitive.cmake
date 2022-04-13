# -*- cmake -*-

# these should be moved to their own cmake file
include(Prebuilt)
include(Boost)

include_guard()

use_prebuilt_binary(colladadom)
use_prebuilt_binary(minizip-ng) # needed for colladadom
use_prebuilt_binary(pcre)
use_prebuilt_binary(libxml2)

create_target( ll::pcre )
set_target_libraries( ll::pcre pcrecpp pcre )

create_target( ll::minizip-ng )
if (WINDOWS)
    set_target_libraries( ll::minizip-ng libminizip )
else()
    set_target_libraries( ll::minizip-ng minizip )
endif()

create_target( ll::libxml )
if (WINDOWS)
    set_target_libraries( ll::libxml libxml2_a)
else()
    set_target_libraries( ll::libxml xml2)
endif()

create_target( ll::colladadom )
set_target_include_dirs( ll::colladadom
        ${LIBS_PREBUILT_DIR}/include/collada
        ${LIBS_PREBUILT_DIR}/include/collada/1.4
        )
if (WINDOWS)
    set_target_libraries(ll::colladadom libcollada14dom23-s ll::libxml ll::minizip-ng )
elseif (DARWIN)
    set_target_libraries(ll::colladadom collada14dom ll::libxml ll::minizip-ng)
elseif (LINUX)
    set_target_libraries(ll::colladadom collada14dom ll::libxml ll::minizip-ng)
endif()