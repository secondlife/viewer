# -*- cmake -*-
include(Prebuilt)

include_guard()
create_target( ll::expat INTERFACE IMPORTED )

use_prebuilt_binary(expat)
if (WINDOWS)
    target_link_libraries( ll::expat  INTERFACE libexpatMT )
    set(EXPAT_COPY libexpatMT.dll)
else (WINDOWS)
    target_link_libraries( ll::expat INTERFACE expat )
    if (DARWIN)
        set(EXPAT_COPY libexpat.1.dylib libexpat.dylib)
    else ()
        set(EXPAT_COPY libexpat.so.1 libexpat.so)
    endif ()
endif (WINDOWS)
target_include_directories( ll::expat SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include )
