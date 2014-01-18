# -*- cmake -*-

include(Boost)
include(EXPAT)

set(LLXML_INCLUDE_DIRS
    ${LIBS_OPEN_DIR}/llxml
    ${EXPAT_INCLUDE_DIRS}
    )
set(LLXML_SYSTEM_INCLUDE_DIRS
    ${Boost_INCLUDE_DIRS}
    )

set(LLXML_LIBRARIES llxml)
