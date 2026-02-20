# -*- cmake -*-
include_guard()
add_library( ll::libcurl INTERFACE IMPORTED )

find_package(CURL REQUIRED)
target_link_libraries(ll::libcurl INTERFACE CURL::libcurl)

if(LINUX OR DARWIN)
    find_library(NGHTTP2_LIBRARIES nghttp2 REQUIRED)
    target_link_libraries(ll::libcurl INTERFACE ${NGHTTP2_LIBRARIES})
endif()
