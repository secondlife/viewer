# -*- cmake -*-
include_guard()

add_library(ll::libvlc INTERFACE IMPORTED)

if(WINDOWS OR DARWIN)
    find_library(LIBVLC_LIBRARY_RELEASE NAMES vlc PATHS "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib" NO_DEFAULT_PATH REQUIRED)
    find_library(LIBVLCCORE_LIBRARY_RELEASE NAMES vlccore PATHS "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib" NO_DEFAULT_PATH REQUIRED)
    target_link_libraries(ll::libvlc INTERFACE ${LIBVLC_LIBRARY_RELEASE} ${LIBVLCCORE_LIBRARY_RELEASE})
    target_include_directories(ll::libvlc SYSTEM INTERFACE ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include)
else()
    find_package(PkgConfig REQUIRED)

    pkg_check_modules(libvlc REQUIRED IMPORTED_TARGET libvlc)
    target_link_libraries(ll::libvlc INTERFACE PkgConfig::libvlc)
endif()
