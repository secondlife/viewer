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
    set(KDU_LIBRARY debug kdu_cored optimized kdu_core)
  else (WINDOWS)
    set(KDU_LIBRARY kdu)
  endif (WINDOWS)

  set(KDU_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/include/kdu)

  set(LLKDU_LIBRARY llkdu)
  set(LLKDU_LIBRARIES ${LLKDU_LIBRARY})
endif (USE_KDU)
