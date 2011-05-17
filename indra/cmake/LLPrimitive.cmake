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
        debug libboost_filesystem-vc100-mt-gd-1_45
        optimized libboost_filesystem-vc100-mt-1_45
        debug libboost_system-vc100-mt-gd-1_45
        optimized libboost_system-vc100-mt-1_45
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

