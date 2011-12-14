# -*- cmake -*-
include(Prebuilt)

use_prebuilt_binary(llpathinglib)
set(LLPATHING_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/libraries/include)


set(LLPATHING_DEBUG_LIBRARY_PATH ${LIBS_PREBUILT_DIR}/lib/debug)
set(LLPATHING_RELEASE_LIBRARY_PATH ${LIBS_PREBUILT_DIR}/lib/release)

find_library(LL_PATHING_DEBUG_LIB llpathinglib PATHS ${LLPATHING_DEBUG_LIBRARY_PATH})
find_library(LL_PATHING_RELEASE_LIB llpathinglib PATHS ${LLPATHING_RELEASE_LIBRARY_PATH})

set(LLPATHING_LIBRARIES

    debug     ${LL_PATHING_DEBUG_LIB}
    optimized ${LL_PATHING_RELEASE_LIB}
)
