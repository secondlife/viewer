# -*- cmake -*-

# USE_KDU can be set when launching cmake as an option using the argument -DUSE_KDU:BOOL=ON
# When building using proprietary binaries though (i.e. having access to LL private servers),
# we always build with KDU
if (INSTALL_PROPRIETARY)
  set(USE_KDU ON CACHE BOOL "Use Kakadu library.")
endif (INSTALL_PROPRIETARY)

include_guard()
add_library( ll::kdu INTERFACE IMPORTED )

if (USE_KDU)
  include(Prebuilt)
  use_prebuilt_binary(kdu)

  if (WINDOWS)
    find_library(KDU_LIBRARY
      NAMES
      kdu
      PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

  else (WINDOWS)
    find_library(KDU_LIBRARY
      NAMES
      libkdu.a
      PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

  endif (WINDOWS)

  target_link_libraries(ll::kdu INTERFACE ${KDU_LIBRARY})

  target_include_directories( ll::kdu SYSTEM INTERFACE
          ${AUTOBUILD_INSTALL_DIR}/include/kdu
          ${LIBS_OPEN_DIR}/llkdu
          )
  target_compile_definitions(ll::kdu INTERFACE KDU_NO_THREADS=1)
endif (USE_KDU)
