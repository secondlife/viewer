# -*- cmake -*-

set(CURL_FIND_QUIETLY ON)
set(CURL_FIND_REQUIRED ON)

if (STANDALONE)
  include(FindCURL)
else (STANDALONE)
  if (WINDOWS)
    set(CURL_LIBRARIES 
    debug libcurld
    optimized libcurl)
  else (WINDOWS)
    set(CURL_LIBRARIES curl)
  endif (WINDOWS)
  set(CURL_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include)
endif (STANDALONE)
