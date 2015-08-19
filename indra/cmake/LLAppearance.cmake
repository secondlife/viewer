# -*- cmake -*-

include(Variables)
include(Boost)
include(LLMessage)
include(LLCoreHttp)

set(LLAPPEARANCE_INCLUDE_DIRS
    ${LIBS_OPEN_DIR}/llappearance
    llcorehttp
    llmessage
    ${BOOST_COROUTINE_LIBRARY}
    ${BOOST_CONTEXT_LIBRARY}
    )

if (BUILD_HEADLESS)
  set(LLAPPEARANCE_HEADLESS_LIBRARIES
    llappearanceheadless
    llcorehttp
    llmessage
    ${BOOST_COROUTINE_LIBRARY}
    ${BOOST_CONTEXT_LIBRARY}
    )
endif (BUILD_HEADLESS)

set(LLAPPEARANCE_LIBRARIES llappearance)


