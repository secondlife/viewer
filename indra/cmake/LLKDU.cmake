# -*- cmake -*-

# USE_KDU can be set when launching cmake as an option using the argument -DUSE_KDU:BOOL=ON
# When building using proprietary binaries though (i.e. having access to LL private servers), 
# we always build with KDU
if (INSTALL_PROPRIETARY)
  set(USE_KDU ON CACHE BOOL "Use Kakadu library.")
endif (INSTALL_PROPRIETARY)

if (USE_KDU)
  include(Prebuilt)
  use_prebuilt_binary(kdu)
  if (WINDOWS)
    set(KDU_LIBRARY kdu.lib)
  else (WINDOWS)
    set(KDU_LIBRARY libkdu.a)
  endif (WINDOWS)
  set(KDU_INCLUDE_DIR ${AUTOBUILD_INSTALL_DIR}/include/kdu)
  set(LLKDU_INCLUDE_DIRS ${LIBS_OPEN_DIR}/llkdu)
  set(LLKDU_LIBRARIES llkdu)
endif (USE_KDU)
