# -*- cmake -*-

include(Variables)
include(Boost)
include(LLCoreHttp)

if (BUILD_HEADLESS)
  set(LLAPPEARANCE_HEADLESS_LIBRARIES
    llappearanceheadless
    )
endif (BUILD_HEADLESS)




