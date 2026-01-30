# -*- cmake -*-
include_guard()
add_library( ll::libcurl INTERFACE IMPORTED )

find_package(CURL REQUIRED)
target_link_libraries(ll::libcurl INTERFACE CURL::libcurl)
