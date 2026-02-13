# -*- cmake -*-
include_guard()

add_library(ll::webrtc INTERFACE IMPORTED)

if(USE_VCPKG)
    find_library(WEBRTC_LIBRARY_RELEASE NAMES webrtc PATHS "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib" NO_DEFAULT_PATH)
    target_link_libraries(ll::webrtc INTERFACE ${WEBRTC_LIBRARY_RELEASE})
target_include_directories(ll::webrtc SYSTEM INTERFACE "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include/webrtc" "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include/webrtc/third_party/abseil-cpp")
    return()
else()
    include(Linking)
    include(Prebuilt)

    target_include_directories( ll::webrtc SYSTEM INTERFACE "${LIBS_PREBUILT_DIR}/include/webrtc" "${LIBS_PREBUILT_DIR}/include/webrtc/third_party/abseil-cpp")
    use_prebuilt_binary(webrtc)

    find_library(WEBRTC_LIBRARY
        NAMES
        webrtc
        PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

    target_link_libraries(ll::webrtc INTERFACE ${WEBRTC_LIBRARY})
endif()

if (DARWIN)
    target_link_libraries(ll::webrtc INTERFACE ll::oslibraries)
elseif (LINUX)
    target_link_libraries(ll::webrtc INTERFACE X11)
endif ()


