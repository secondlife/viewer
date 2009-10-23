# -*- cmake -*-
include(Prebuilt)

if (INSTALL_PROPRIETARY AND NOT STANDALONE AND EXISTS ${LIBS_CLOSED_DIR}/llkdu)
  use_prebuilt_binary(kdu)
  if (WINDOWS)
    set(KDU_LIBRARY debug kdu_cored optimized kdu_core)
  else (WINDOWS)
    set(KDU_LIBRARY kdu)
  endif (WINDOWS)

  set(KDU_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/include)

  set(LLKDU_LIBRARY llkdu)
  set(LLKDU_STATIC_LIBRARY llkdu_static)
  set(LLKDU_LIBRARIES ${LLKDU_LIBRARY})
  set(LLKDU_STATIC_LIBRARIES ${LLKDU_STATIC_LIBRARY})
endif (INSTALL_PROPRIETARY AND NOT STANDALONE AND EXISTS ${LIBS_CLOSED_DIR}/llkdu)
