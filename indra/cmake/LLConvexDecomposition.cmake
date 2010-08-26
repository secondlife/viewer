# -*- cmake -*-
include(Prebuilt)

set(LLCONVEXDECOMP_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include)
  
if (INSTALL_PROPRIETARY AND NOT STANDALONE)
  use_prebuilt_binary(llconvexdecomposition)
  if (WINDOWS)
    set(LLCONVEXDECOMP_LIBRARY llconvexdecomposition)
  else (WINDOWS)
    set(LLCONVEXDECOMP_LIBRARY llconvexdecompositionstub)
  endif (WINDOWS)
else (INSTALL_PROPRIETARY AND NOT STANDALONE)
  use_prebuilt_binary(llconvexdecompositionstub)
  set(LLCONVEXDECOMP_LIBRARY llconvexdecompositionstub)
endif (INSTALL_PROPRIETARY AND NOT STANDALONE)
