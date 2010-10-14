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
        llprimitive
        debug libcollada14dom21-d
        debug libboost_filesystem-d
        debug libboost_system-d
        optimized libcollada14dom21
        optimized libboost_filesystem
        optimized libboost_system
        )
else (WINDOWS)
    set(LLPRIMITIVE_LIBRARIES 
        llprimitive
        collada14dom
        xml2
        pcrecpp
        pcre
        )
endif (WINDOWS)

