# -*- cmake -*-
include(Prebuilt)

if (STANDALONE)
  include(FindPkgConfig)

  pkg_check_modules(FREETYPE REQUIRED freetype2)
else (STANDALONE)
  use_prebuilt_binary(freetype)
  if (LINUX)
    set(FREETYPE_INCLUDE_DIRS
        ${LIBS_PREBUILT_DIR}/include)
  else (LINUX)
    set(FREETYPE_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include)
  endif (LINUX)

  set(FREETYPE_LIBRARIES freetype)
endif (STANDALONE)

link_directories(${FREETYPE_LIBRARY_DIRS})
