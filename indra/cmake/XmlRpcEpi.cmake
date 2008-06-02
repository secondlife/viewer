# -*- cmake -*-

set(XMLRPCEPI_FIND_QUIETLY ON)
set(XMLRPCEPI_FIND_REQUIRED ON)

if (STANDALONE)
  include(FindXmlRpcEpi)
else (STANDALONE)
    if (WINDOWS)
        set(XMLRPCEPI_LIBRARIES xmlrpcepi)
    else (WINDOWS)
        set(XMLRPCEPI_LIBRARIES xmlrpc-epi)
    endif (WINDOWS)
    set(XMLRPCEPI_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include)
endif (STANDALONE)
