# -*- cmake -*-
include(Prebuilt)

if (NOT STANDALONE)
  use_prebuilt_binary(libhunspell)
  use_prebuilt_binary(libuuid)
  use_prebuilt_binary(slvoice)
  use_prebuilt_binary(fontconfig)
endif(NOT STANDALONE)

if(VIEWER AND NOT STANDALONE)
  if(EXISTS ${CMAKE_SOURCE_DIR}/newview/res/have_artwork_bundle.marker)
    message(STATUS "We seem to have an artwork bundle in the tree - brilliant.")
  else(EXISTS ${CMAKE_SOURCE_DIR}/newview/res/have_artwork_bundle.marker)
    message(FATAL_ERROR "Didn't find an artwork bundle - this needs to be downloaded separately and unpacked into this tree.  You can probably get it from the same place you got your viewer source.  Thanks!")
  endif(EXISTS ${CMAKE_SOURCE_DIR}/newview/res/have_artwork_bundle.marker)
endif(VIEWER AND NOT STANDALONE)
