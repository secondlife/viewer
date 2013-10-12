# -*- cmake -*-

# these should be moved to their own cmake file
include(Prebuilt)
include(Boost)

use_prebuilt_binary(colladadom)
use_prebuilt_binary(pcre)

set(LLPRIMITIVE_INCLUDE_DIRS
    ${LIBS_OPEN_DIR}/llprimitive
    )
if (WINDOWS)
    set(LLPRIMITIVE_LIBRARIES 
        debug llprimitive
        optimized llprimitive
        debug libcollada14dom22-d
        optimized libcollada14dom22
        ${BOOST_SYSTEM_LIBRARIES}
        )
elseif (LINUX)
    use_prebuilt_binary(libxml2)
    set(LLPRIMITIVE_LIBRARIES
        llprimitive
        collada14dom
        minizip
        xml2
        pcrecpp
        pcre
        )
else (WINDOWS)
    set(LLPRIMITIVE_LIBRARIES 
        llprimitive
        collada14dom
        minizip
        xml2
        pcrecpp
        pcre
        )
endif (WINDOWS)

