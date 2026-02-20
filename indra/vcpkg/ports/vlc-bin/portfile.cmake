set(VCPKG_POLICY_ALLOW_OBSOLETE_MSVCRT enabled)
set(VCPKG_POLICY_MISMATCHED_NUMBER_OF_BINARIES enabled)
set(VCPKG_FIXUP_MACHO_RPATH OFF)

if(VCPKG_TARGET_IS_WINDOWS)
    vcpkg_download_distfile(
        VLC_ARCHIVE
        URLS https://github.com/secondlife/3p-vlc-bin/releases/download/v3.0.21.296d9f4/vlc_bin-3.0.21.11968962952-windows64-11968962952.tar.zst
        FILENAME vlc-win64.tar.zst
        SHA512 22641ec278317b3a00549aa4b8b1e0606e5c8e2ca9c5a830c4254adc3fe0674bce15d8bbff51aa40f3c48482553ff9a5d654fa69a6a23d7a10abfa8236d1dce5
    )
elseif(VCPKG_TARGET_IS_OSX)
    vcpkg_download_distfile(
        VLC_ARCHIVE
        URLS https://github.com/secondlife/3p-vlc-bin/releases/download/v3.0.21.296d9f4/vlc_bin-3.0.21.11968962952-darwin64-11968962952.tar.zst
        FILENAME vlc-darwin64.tar.zst
        SHA512 f6ce2fa90d07a2a3f229f28516e9e77f180cb9783217c4a6b790e03f90a81efdc890dce422f5e4a584d04f7f3b73089a4d7bc04cf2129268feee6243e69bc1bf
    )
endif()

vcpkg_extract_source_archive(VLC_DIR ARCHIVE ${VLC_ARCHIVE} NO_REMOVE_ONE_LEVEL)

file(INSTALL
    DIRECTORY "${VLC_DIR}/include/vlc/"
    DESTINATION "${CURRENT_PACKAGES_DIR}/include/vlc"
    FILES_MATCHING
    PATTERN "*.h"
)

if(VCPKG_TARGET_IS_WINDOWS)
    file(INSTALL "${VLC_DIR}/lib/release/libvlc.lib" DESTINATION "${CURRENT_PACKAGES_DIR}/lib")
    file(INSTALL "${VLC_DIR}/lib/release/libvlccore.lib" DESTINATION "${CURRENT_PACKAGES_DIR}/lib")

    file(INSTALL "${VLC_DIR}/bin/release/libvlc.dll" DESTINATION "${CURRENT_PACKAGES_DIR}/bin")
    file(INSTALL "${VLC_DIR}/bin/release/libvlccore.dll" DESTINATION "${CURRENT_PACKAGES_DIR}/bin")

    file(INSTALL
        DIRECTORY "${VLC_DIR}/bin/release/plugins/"
        DESTINATION "${CURRENT_PACKAGES_DIR}/plugins/${PORT}/plugins"
        FILES_MATCHING
        PATTERN "*.dll"
        PATTERN "*.so"
        PATTERN "*.dat"
    )
elseif(VCPKG_TARGET_IS_OSX)
    file(INSTALL "${VLC_DIR}/lib/release/libvlc.dylib" DESTINATION "${CURRENT_PACKAGES_DIR}/lib" FOLLOW_SYMLINK_CHAIN)
    file(INSTALL "${VLC_DIR}/lib/release/libvlccore.dylib" DESTINATION "${CURRENT_PACKAGES_DIR}/lib" FOLLOW_SYMLINK_CHAIN)

    file(INSTALL
        DIRECTORY "${VLC_DIR}/lib/release/plugins/"
        DESTINATION "${CURRENT_PACKAGES_DIR}/plugins/${PORT}/plugins"
        FILES_MATCHING
        PATTERN "*.dylib"
        PATTERN "*.so"
        PATTERN "*.dat"
    )
endif()

vcpkg_install_copyright(FILE_LIST "${VLC_DIR}/LICENSES/vlc.txt")
