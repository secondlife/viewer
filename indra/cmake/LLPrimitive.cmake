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
        libcollada14dom21
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

