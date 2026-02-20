set(VCPKG_POLICY_ALLOW_EMPTY_FOLDERS enabled)
set(VCPKG_POLICY_MISMATCHED_NUMBER_OF_BINARIES enabled)
set(VCPKG_FIXUP_MACHO_RPATH OFF)
set(VCPKG_FIXUP_ELF_RPATH OFF)

if(VCPKG_TARGET_IS_WINDOWS)
    vcpkg_download_distfile(
        DULLAHAN_ARCHIVE
        URLS https://github.com/secondlife/dullahan/releases/download/v1.26.0-CEF_139.0.40/dullahan-1.26.0.202510161628_139.0.40_g465474a_chromium-139.0.7258.139-windows64-18568015445.tar.zst
        FILENAME dullahan-windows64.tar.zst
        SHA512 8b2fe2d227c58561ad3159be4feb7682e0a3eece4c8b1ca982759820162d9fcff029ba238219e800296db582370f33d7e02772730f199837b294a58c95aaa36e
    )

    vcpkg_extract_source_archive(
        DULLAHAN_DIR
        ARCHIVE ${DULLAHAN_ARCHIVE}
        NO_REMOVE_ONE_LEVEL
    )

    file(INSTALL
        DIRECTORY "${DULLAHAN_DIR}/include/cef/"
        DESTINATION "${CURRENT_PACKAGES_DIR}/include/cef"
        FILES_MATCHING
        PATTERN "*.h"
    )

    file(INSTALL
        DIRECTORY "${DULLAHAN_DIR}/lib/release/"
        DESTINATION "${CURRENT_PACKAGES_DIR}/lib"
        FILES_MATCHING
        PATTERN "*.lib"
    )

    file(INSTALL "${DULLAHAN_DIR}/bin/release/libcef.dll" DESTINATION "${CURRENT_PACKAGES_DIR}/bin")

    file(INSTALL
        DIRECTORY "${DULLAHAN_DIR}/bin/release/"
        DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}/bin"
        FILES_MATCHING
        PATTERN "*.*"
        PATTERN "libcef.dll" EXCLUDE
        PATTERN "libcef.lib" EXCLUDE
    )

    file(INSTALL
        DIRECTORY "${DULLAHAN_DIR}/resources/"
        DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}/resources"
        FILES_MATCHING
        PATTERN "*.*"
    )
elseif(VCPKG_TARGET_IS_OSX)
    vcpkg_download_distfile(
        DULLAHAN_ARCHIVE
        URLS https://github.com/secondlife/dullahan/releases/download/v1.26.0-CEF_139.0.40/dullahan-1.26.0.202510161627_139.0.40_g465474a_chromium-139.0.7258.139-darwin64-18568015445.tar.zst
        FILENAME dullahan-osx.tar.zst
        SHA512 e847f119605bd69cc47bf08ca06abfc7e745b5e0e0ccbdd84447b2df5e4852d0de3456ef82ac60fc4cbc4b17622c5300273cbcec496236e1573ed0afa5ea4f3d
    )

    vcpkg_extract_source_archive(
        DULLAHAN_DIR
        ARCHIVE ${DULLAHAN_ARCHIVE}
        NO_REMOVE_ONE_LEVEL
    )

    # Create output directories and move files into place
    file(MAKE_DIRECTORY "${CURRENT_PACKAGES_DIR}/include/")
    file(MAKE_DIRECTORY "${CURRENT_PACKAGES_DIR}/lib/")
    file(MAKE_DIRECTORY "${CURRENT_PACKAGES_DIR}/share/${PORT}/helpers/")

    set(CEF_FRAMEWORK_DIR "${CURRENT_PACKAGES_DIR}/lib/Chromium Embedded Framework.framework")
    file(RENAME "${DULLAHAN_DIR}/lib/release/Chromium Embedded Framework.framework" "${CEF_FRAMEWORK_DIR}")

    file(RENAME "${DULLAHAN_DIR}/lib/release/DullahanHelper.app" "${CURRENT_PACKAGES_DIR}/share/${PORT}/helpers/DullahanHelper.app")
    file(RENAME "${DULLAHAN_DIR}/lib/release/DullahanHelper (Alerts).app" "${CURRENT_PACKAGES_DIR}/share/${PORT}/helpers/DullahanHelper (Alerts).app")
    file(RENAME "${DULLAHAN_DIR}/lib/release/DullahanHelper (GPU).app" "${CURRENT_PACKAGES_DIR}/share/${PORT}/helpers/DullahanHelper (GPU).app")
    file(RENAME "${DULLAHAN_DIR}/lib/release/DullahanHelper (Plugin).app" "${CURRENT_PACKAGES_DIR}/share/${PORT}/helpers/DullahanHelper (Plugin).app")
    file(RENAME "${DULLAHAN_DIR}/lib/release/DullahanHelper (Renderer).app" "${CURRENT_PACKAGES_DIR}/share/${PORT}/helpers/DullahanHelper (Renderer).app")

    file(INSTALL "${DULLAHAN_DIR}/lib/release/libcef_dll_wrapper.a" DESTINATION "${CURRENT_PACKAGES_DIR}/lib")
    file(INSTALL "${DULLAHAN_DIR}/lib/release/libdullahan.a" DESTINATION "${CURRENT_PACKAGES_DIR}/lib")

    file(INSTALL
        DIRECTORY "${DULLAHAN_DIR}/include/cef/"
        DESTINATION "${CURRENT_PACKAGES_DIR}/include/cef"
        FILES_MATCHING
        PATTERN "*.h"
    )
elseif(VCPKG_TARGET_IS_LINUX)
    vcpkg_download_distfile(
        DULLAHAN_ARCHIVE
        URLS https://github.com/secondlife/dullahan/releases/download/v1.26.0-CEF_139.0.40/dullahan-1.26.0.202510161627_139.0.40_g465474a_chromium-139.0.7258.139-linux64-18568015445.tar.zst
        FILENAME dullahan-osx.tar.zst
        SHA512 12177c6dda56a4a91a8cf23748269ce765b794290018c3ae254d0b11430451ead835d9f252cddd7bd258c58502297fafd45252deefdafeb24c8d6e8f7ae6a3ee
    )

    vcpkg_extract_source_archive(
        DULLAHAN_DIR
        ARCHIVE ${DULLAHAN_ARCHIVE}
        NO_REMOVE_ONE_LEVEL
    )

    file(INSTALL
        DIRECTORY "${DULLAHAN_DIR}/lib/release/"
        DESTINATION "${CURRENT_PACKAGES_DIR}/lib"
        USE_SOURCE_PERMISSIONS
        FILES_MATCHING
        PATTERN "*.a"
        PATTERN "*.so*"
        PATTERN "*.json*"
        PATTERN "*.bin*"
    )

    file(INSTALL
        DIRECTORY "${DULLAHAN_DIR}/bin/release/"
        DESTINATION "${CURRENT_PACKAGES_DIR}/bin"
        USE_SOURCE_PERMISSIONS
        FILES_MATCHING
        PATTERN "chrome-sandbox"
        PATTERN "dullahan_host"
    )

    file(INSTALL
        DIRECTORY "${DULLAHAN_DIR}/include/cef/"
        DESTINATION "${CURRENT_PACKAGES_DIR}/include/cef"
        FILES_MATCHING
        PATTERN "*.h"
    )

    file(INSTALL
        DIRECTORY "${DULLAHAN_DIR}/resources/"
        DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}/resources"
        FILES_MATCHING
        PATTERN "*.*"
    )
endif()

vcpkg_install_copyright(FILE_LIST ${DULLAHAN_DIR}/LICENSES/LICENSE.txt ${DULLAHAN_DIR}/LICENSES/CEF_LICENSE.txt)
