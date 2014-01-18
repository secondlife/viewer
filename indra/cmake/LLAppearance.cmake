# -*- cmake -*-

include(Variables)

set(LLAPPEARANCE_INCLUDE_DIRS
    ${LIBS_OPEN_DIR}/llappearance
    )

if (BUILD_HEADLESS)
  set(LLAPPEARANCE_HEADLESS_LIBRARIES
    llappearanceheadless
    )
endif (BUILD_HEADLESS)

set(LLAPPEARANCE_LIBRARIES llappearance)


