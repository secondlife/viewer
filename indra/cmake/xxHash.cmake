# -*- cmake -*-
if (XXHASH_CMAKE_INCLUDED)
  return()
endif (XXHASH_CMAKE_INCLUDED)
set (XXHASH_CMAKE_INCLUDED TRUE)

include(Prebuilt)
use_prebuilt_binary(xxhash)
