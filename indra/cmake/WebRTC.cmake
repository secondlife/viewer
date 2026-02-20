# -*- cmake -*-
include_guard()

add_library(ll::webrtc INTERFACE IMPORTED)

find_library(WEBRTC_LIBRARY_RELEASE NAMES webrtc PATHS "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib" REQUIRED NO_DEFAULT_PATH)
target_link_libraries(ll::webrtc INTERFACE ${WEBRTC_LIBRARY_RELEASE})
target_include_directories(ll::webrtc SYSTEM INTERFACE "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include/webrtc" "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include/webrtc/third_party/abseil-cpp")

if (DARWIN)
    target_link_libraries(ll::webrtc INTERFACE ll::oslibraries)
elseif (LINUX)
    target_link_libraries(ll::webrtc INTERFACE X11)
endif ()


