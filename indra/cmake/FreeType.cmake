# -*- cmake -*-

if (STANDALONE)
  include(FindPkgConfig)

  pkg_check_modules(FREETYPE REQUIRED freetype2)
else (STANDALONE)
  if (LINUX)
    set(FREETYPE_INCLUDE_DIRS
        ${LIBS_PREBUILT_DIR}/${LL_ARCH_DIR}/include)
  else (LINUX)
    set(FREETYPE_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include)
  endif (LINUX)

  set(FREETYPE_LIBRARIES freetype)
endif (STANDALONE)

link_directories(${FREETYPE_LIBRARY_DIRS})
