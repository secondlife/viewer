# -*- cmake -*-
include(Prebuilt)

set(LLPHYSICS_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/lib/include)

if (INSTALL_PROPRIETARY AND NOT STANDALONE)
   use_prebuilt_binary(llphysicsextensions)

   set(LLPHYSICS_DEBUG_LIBRARY_PATH ${LIBS_PREBUILT_DIR}/lib/debug)
   set(LLPHYSICS_RELEASE_LIBRARY_PATH ${LIBS_PREBUILT_DIR}/lib/release)

   find_library(LL_PHYSICS_DEBUG_LIB llphysicsextensions PATHS ${LLPHYSICS_DEBUG_LIBRARY_PATH})
   find_library(LL_PHYSICS_RELEASE_LIB llphysicsextensions PATHS ${LLPHYSICS_RELEASE_LIBRARY_PATH})

   set(LLPHYSICS_LIBRARIES
       debug     ${LL_PHYSICS_DEBUG_LIB}
       optimized ${LL_PHYSICS_RELEASE_LIB}
   )

   if (LINUX)
       list(INSERT LLPHYSICS_LIBRARIES 0 -Wl,--start-group)
       list(APPEND LLPHYSICS_LIBRARIES -Wl,--end-group)
   endif (LINUX)
else (INSTALL_PROPRIETARY AND NOT STANDALONE)
endif (INSTALL_PROPRIETARY AND NOT STANDALONE)
