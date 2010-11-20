# -*- cmake -*-
include(Prebuilt)

# USE_KDU can be set when launching cmake or develop.py as an option using the argument -DUSE_KDU:BOOL=ON
# When building using proprietary binaries though (i.e. having access to LL private servers), we always build with KDU
if (INSTALL_PROPRIETARY AND NOT STANDALONE)
  set(USE_KDU ON)
endif (INSTALL_PROPRIETARY AND NOT STANDALONE)

if (USE_KDU)
  use_prebuilt_binary(kdu)
  if (WINDOWS)
    set(KDU_LIBRARY debug kdud.lib optimized kdu.lib)
  else (WINDOWS)
    set(KDU_LIBRARY libkdu.a)
  endif (WINDOWS)
  set(KDU_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/include/kdu)
  set(LLKDU_INCLUDE_DIRS ${LIBS_OPEN_DIR}/llkdu)
  set(LLKDU_LIBRARIES llkdu)
endif (USE_KDU)
