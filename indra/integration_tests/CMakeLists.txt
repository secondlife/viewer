# -*- cmake -*-

add_subdirectory(llui_libtest)
if (LLIMAGE_LIBTEST)
  add_subdirectory(llimage_libtest)
endif (LLIMAGE_LIBTEST)
