# -*- cmake -*-
include(Linking)
include(Prebuilt)

include_guard()

set(USE_OPENAL ON CACHE BOOL "Enable OpenAL")
# ND: To streamline arguments passed, switch from OPENAL to USE_OPENAL
# To not break all old build scripts convert old arguments but warn about it
if(OPENAL)
  message( WARNING "Use of the OPENAL argument is deprecated, please switch to USE_OPENAL")
  set(USE_OPENAL ${OPENAL})
endif()

if (USE_OPENAL)
  add_library( ll::openal INTERFACE IMPORTED )
  target_include_directories( ll::openal SYSTEM INTERFACE "${LIBS_PREBUILT_DIR}/include/AL")
  target_compile_definitions( ll::openal INTERFACE LL_OPENAL=1)
  use_prebuilt_binary(openal)

  if(WINDOWS)
    target_link_libraries( ll::openal INTERFACE
            ${ARCH_PREBUILT_DIRS_RELEASE}/OpenAL32.lib
            ${ARCH_PREBUILT_DIRS_RELEASE}/alut.lib
            )
  elseif(LINUX)
    target_link_libraries( ll::openal INTERFACE
            ${ARCH_PREBUILT_DIRS_RELEASE}/libopenal.so
            ${ARCH_PREBUILT_DIRS_RELEASE}/libalut.so
            )
  else()
    target_link_libraries( ll::openal INTERFACE
            ${ARCH_PREBUILT_DIRS_RELEASE}/libopenal.dylib
            ${ARCH_PREBUILT_DIRS_RELEASE}/libalut.dylib
            )
  endif()
endif ()
