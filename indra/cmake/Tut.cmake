# -*- cmake -*-
include(Prebuilt)

set(TUT_FIND_REQUIRED TRUE)
set(TUT_FIND_QUIETLY TRUE)

if (STANDALONE)
  include(FindTut)
else (STANDALONE)
  use_prebuilt_binary(tut)
endif (STANDALONE)
