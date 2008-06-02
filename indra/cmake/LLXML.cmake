# -*- cmake -*-

include(EXPAT)

set(LLXML_INCLUDE_DIRS
    ${LIBS_OPEN_DIR}/llxml
    ${EXPAT_INCLUDE_DIRS}
    )

set(LLXML_LIBRARIES
    llxml
    ${EXPAT_LIBRARIES}
    )
