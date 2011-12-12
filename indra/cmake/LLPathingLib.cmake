# -*- cmake -*-
include(Prebuilt)

use_prebuilt_binary(llpathinglib)
set(LLPATHING_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/libraries/include)
if (CMAKE_BUILD_TYPE MATCHES "Debug")
   set(LLPATHING_LIBRARY_PATH ${LIBS_PREBUILT_DIR}/lib/debug)
else (CMAKE_BUILD_TYPE MATCHES "Debug")
   set(LLPATHING_LIBRARY_PATH ${LIBS_PREBUILT_DIR}/lib/release)
endif (CMAKE_BUILD_TYPE MATCHES "Debug")

find_library(LL_PATHING_LIB llpathinglib PATHS ${LLPATHING_LIBRARY_PATH})

set(LLPATHING_LIBRARIES
    ${LL_PATHING_LIB}
)
