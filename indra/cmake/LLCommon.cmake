# -*- cmake -*-

include(APR)
include(Boost)
include(EXPAT)
include(ZLIB)

set(LLCOMMON_INCLUDE_DIRS
    ${LIBS_OPEN_DIR}/llcommon
    ${APRUTIL_INCLUDE_DIR}
    ${APR_INCLUDE_DIR}
    ${Boost_INCLUDE_DIRS}
    )

set(LLCOMMON_LIBRARIES llcommon)

set(LLCOMMON_LINK_SHARED OFF CACHE BOOL "Build the llcommon target as a shared library.")
if(LLCOMMON_LINK_SHARED)
    add_definitions(-DLL_COMMON_LINK_SHARED=1)
endif(LLCOMMON_LINK_SHARED)
