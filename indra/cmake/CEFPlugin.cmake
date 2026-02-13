# -*- cmake -*-
include_guard()
add_library(ll::cef INTERFACE IMPORTED)

if(USE_VCPKG)
    find_library(LIBCEF_LIBRARY_RELEASE NAMES cef PATHS "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib" NO_DEFAULT_PATH)
    find_library(LIBCEF_DLL_WRAPPER_LIBRARY_RELEASE NAMES cef_dll_wrapper PATHS "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib" NO_DEFAULT_PATH)
    find_library(DULLAHAN_LIBRARY_RELEASE NAMES dullahan PATHS "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib" NO_DEFAULT_PATH)
    target_link_libraries(ll::cef INTERFACE ${DULLAHAN_LIBRARY_RELEASE} ${LIBCEF_DLL_WRAPPER_LIBRARY_RELEASE} ${LIBCEF_LIBRARY_RELEASE})
    return()
endif()

include(Linking)
include(Prebuilt)

use_prebuilt_binary(dullahan)
target_include_directories( ll::cef SYSTEM INTERFACE  ${LIBS_PREBUILT_DIR}/include/cef)

if (WINDOWS)
    target_link_libraries( ll::cef INTERFACE
        ${ARCH_PREBUILT_DIRS_RELEASE}/libcef.lib
        ${ARCH_PREBUILT_DIRS_RELEASE}/libcef_dll_wrapper.lib
        ${ARCH_PREBUILT_DIRS_RELEASE}/dullahan.lib
    )
elseif (DARWIN)
    FIND_LIBRARY(APPKIT_LIBRARY AppKit)
    if (NOT APPKIT_LIBRARY)
        message(FATAL_ERROR "AppKit not found")
    endif()

    set(CEF_LIBRARY "'${ARCH_PREBUILT_DIRS_RELEASE}/Chromium\ Embedded\ Framework.framework'")
    if (NOT CEF_LIBRARY)
        message(FATAL_ERROR "CEF not found")
    endif()

    target_link_libraries( ll::cef INTERFACE
        ${ARCH_PREBUILT_DIRS_RELEASE}/libcef_dll_wrapper.a
        ${ARCH_PREBUILT_DIRS_RELEASE}/libdullahan.a
        ${APPKIT_LIBRARY}
       )

elseif (LINUX)
    target_link_libraries( ll::cef INTERFACE
            ${ARCH_PREBUILT_DIRS_RELEASE}/libdullahan.a
            ${ARCH_PREBUILT_DIRS_RELEASE}/libcef.so
            ${ARCH_PREBUILT_DIRS_RELEASE}/libcef_dll_wrapper.a
    )
endif (WINDOWS)

target_include_directories(ll::cef SYSTEM INTERFACE ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include/cef)
