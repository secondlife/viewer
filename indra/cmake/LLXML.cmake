# -*- cmake -*-

include(Boost)
include(EXPAT)

set(LLXML_INCLUDE_DIRS
    ${LIBS_OPEN_DIR}/llxml
    ${Boost_INCLUDE_DIRS}
    ${EXPAT_INCLUDE_DIRS}
    )

set(LLXML_LIBRARIES llxml)
