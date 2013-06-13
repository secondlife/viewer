# -*- cmake -*-

include(Variables)
include(FreeType)
include(GLH)

set(LLRENDER_INCLUDE_DIRS
    ${LIBS_OPEN_DIR}/llrender
    ${GLH_INCLUDE_DIR}
    )

if (BUILD_HEADLESS)
  set(LLRENDER_HEADLESS_LIBRARIES
    llrenderheadless
    )
endif (BUILD_HEADLESS)
set(LLRENDER_LIBRARIES
    llrender
    )

