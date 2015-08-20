# -*- cmake -*-

include(Variables)
include(Boost)
include(LLMessage)
include(LLCoreHttp)

set(LLAPPEARANCE_INCLUDE_DIRS
    ${LIBS_OPEN_DIR}/llappearance
    )

if (BUILD_HEADLESS)
  set(LLAPPEARANCE_HEADLESS_LIBRARIES
    llappearanceheadless
    )
endif (BUILD_HEADLESS)

set(LLAPPEARANCE_LIBRARIES llappearance
    llmessage
    llcorehttp
    ${BOOST_COROUTINE_LIBRARY}
    ${BOOST_CONTEXT_LIBRARY}
    ${BOOST_SYSTEM_LIBRARY}
    )



