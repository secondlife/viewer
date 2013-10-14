# -*- cmake -*-
add_subdirectory(llui_libtest)
IF (LLIMAGE_LIBTEST)
  MESSAGE(STATUS "Build llimage_libtest")
  add_subdirectory(llimage_libtest)
ELSE (LLIMAGE_LIBTEST)
  MESSAGE(STATUS "Skip llimage_libtest")
ENDIF (LLIMAGE_LIBTEST)
