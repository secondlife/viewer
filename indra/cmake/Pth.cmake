# -*- cmake -*-
include(Prebuilt)

set(PTH_FIND_QUIETLY ON)
set(PTH_FIND_REQUIRED ON)

if (STANDALONE)
#  ?? How would I construct FindPTH.cmake? This file was cloned from
#  CURL.cmake, which uses include(FindCURL), but there's no FindCURL.cmake?
#  include(FindPTH)
else (STANDALONE)
  # This library is only needed to support Boost.Coroutine, and only on Mac.
  if (DARWIN)
    use_prebuilt_binary(pth)
    set(PTH_LIBRARIES pth)
    set(PTH_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include)
  else (DARWIN)
    set(PTH_LIBRARIES)
    set(PTH_INCLUDE_DIRS)
  endif (DARWIN)
endif (STANDALONE)
