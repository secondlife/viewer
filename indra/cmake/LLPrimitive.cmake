# -*- cmake -*-

# these should be moved to their own cmake file
include(Prebuilt)
use_prebuilt_binary(colladadom)
use_prebuilt_binary(pcre)
use_prebuilt_binary(libxml)

set(LLPRIMITIVE_INCLUDE_DIRS
    ${LIBS_OPEN_DIR}/llprimitive
    )
if (WINDOWS)
    set(LLPRIMITIVE_LIBRARIES 
        debug llprimitive
        optimized llprimitive
        debug libcollada14dom22-d
        optimized libcollada14dom22
        debug libboost_filesystem-mt-gd
        optimized libboost_filesystem-mt
        debug libboost_system-mt-gd
        optimized libboost_system-mt
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

