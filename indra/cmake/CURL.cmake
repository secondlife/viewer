# -*- cmake -*-
include_guard()
add_library( ll::libcurl INTERFACE IMPORTED )

find_package(CURL REQUIRED)
target_link_libraries(ll::libcurl INTERFACE CURL::libcurl)

if(LINUX OR DARWIN)
    find_library(NGHTTPS_LIBRARIES nghttp2 REQUIRED)
    target_link_libraries(ll::libcurl INTERFACE ${NGHTTPS_LIBRARIES})
endif()
