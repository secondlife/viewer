# -*- cmake -*-
include(Prebuilt)

set(EXPAT_FIND_QUIETLY ON)
set(EXPAT_FIND_REQUIRED ON)

if (STANDALONE)
  include(FindEXPAT)
else (STANDALONE)
    use_prebuilt_binary(expat)
    if (WINDOWS)
        set(EXPAT_LIBRARIES libexpatMT)
    else (WINDOWS)
        set(EXPAT_LIBRARIES expat)
    endif (WINDOWS)
    set(EXPAT_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include)
endif (STANDALONE)
