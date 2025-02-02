# -*- cmake -*-
include(Linking)
include(Prebuilt)

include_guard()

# ND: Turn this off by default, the openal code in the viewer isn't very well maintained, seems
# to have memory leaks, has no option to play music streams
# It probably makes sense to to completely remove it

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

  find_library(OPENAL_LIBRARY
      NAMES
      OpenAL32
      openal
      PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

  find_library(ALUT_LIBRARY
      NAMES
      alut
      PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

  target_link_libraries(ll::openal INTERFACE ${OPENAL_LIBRARY} ${ALUT_LIBRARY})

endif ()
