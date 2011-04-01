# -*- cmake -*-
include(Prebuilt)

set(XMLRPCEPI_FIND_QUIETLY ON)
set(XMLRPCEPI_FIND_REQUIRED ON)

if (STANDALONE)
  include(FindXmlRpcEpi)
else (STANDALONE)
    use_prebuilt_binary(xmlrpc-epi)
    if (WINDOWS)
        set(XMLRPCEPI_LIBRARIES 
            debug xmlrpc-epid
            optimized xmlrpc-epi
        )
    else (WINDOWS)
        set(XMLRPCEPI_LIBRARIES xmlrpc-epi)
    endif (WINDOWS)
    set(XMLRPCEPI_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include)
endif (STANDALONE)
