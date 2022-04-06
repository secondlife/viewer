# -*- cmake -*-

# USE_KDU can be set when launching cmake as an option using the argument -DUSE_KDU:BOOL=ON
# When building using proprietary binaries though (i.e. having access to LL private servers), 
# we always build with KDU
if (INSTALL_PROPRIETARY)
  set(USE_KDU ON CACHE BOOL "Use Kakadu library.")
endif (INSTALL_PROPRIETARY)

if( TARGET kdu::kdu )
  return()
endif()
create_target( kdu::kdu )

if (USE_KDU)
  include(Prebuilt)
  use_prebuilt_binary(kdu)
  if (WINDOWS)
    set_target_libraries( kdu::kdu kdu.lib)
  else (WINDOWS)
    set_target_libraries( kdu::kdu libkdu.a)
  endif (WINDOWS)

  set_target_include_dirs( kdu::kdu
          ${AUTOBUILD_INSTALL_DIR}/include/kdu
          ${LIBS_OPEN_DIR}/llkdu
          )
endif (USE_KDU)
