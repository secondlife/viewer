# -*- cmake -*-

include(APR)
include(Boost)
include(EXPAT)
include(Tracy)
include(xxHash)
include(ZLIBNG)

set(LLCOMMON_INCLUDE_DIRS
    ${LIBS_OPEN_DIR}/llcommon
    ${APRUTIL_INCLUDE_DIR}
    ${APR_INCLUDE_DIR}
    ${TRACY_INCLUDE_DIR}
    )
set(LLCOMMON_SYSTEM_INCLUDE_DIRS
    ${Boost_INCLUDE_DIRS}
    )

if (LINUX)
    # In order to support using ld.gold on linux, we need to explicitely
    # specify all libraries that llcommon uses.
    # llcommon uses `clock_gettime' which is provided by librt on linux.
    set(LLCOMMON_LIBRARIES llcommon 
        ${BOOST_FIBER_LIBRARY} 
        ${BOOST_CONTEXT_LIBRARY} 
        ${BOOST_THREAD_LIBRARY} 
        ${BOOST_SYSTEM_LIBRARY} 
        rt
        )
else (LINUX)
    set(LLCOMMON_LIBRARIES llcommon
        ${BOOST_FIBER_LIBRARY} 
        ${BOOST_CONTEXT_LIBRARY} 
        ${BOOST_THREAD_LIBRARY} 
        ${BOOST_SYSTEM_LIBRARY}
        )
endif (LINUX)

set(LLCOMMON_LINK_SHARED OFF CACHE BOOL "Build the llcommon target as a static library.")
if(LLCOMMON_LINK_SHARED)
  add_definitions(-DLL_COMMON_LINK_SHARED=1)
endif(LLCOMMON_LINK_SHARED)
