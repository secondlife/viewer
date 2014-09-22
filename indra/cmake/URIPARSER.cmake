# -*- cmake -*-

set(URIPARSER_FIND_QUIETLY ON)
set(URIPARSER_FIND_REQUIRED ON)

include(Prebuilt)

if (STANDALONE)
  include(FindURIPARSER)
else (STANDALONE)
  use_prebuilt_binary(uriparser)
  if (WINDOWS)
    set(URIPARSER_LIBRARIES 
      debug uriparserd
      optimized uriparser)
  else (WINDOWS)
    set(URIPARSER_LIBRARIES z)
  endif (WINDOWS)
  if (WINDOWS OR LINUX)
    set(URIPARSER_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include/uriparser)
  endif (WINDOWS OR LINUX)
endif (STANDALONE)
