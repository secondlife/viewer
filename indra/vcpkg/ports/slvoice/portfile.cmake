set(VCPKG_POLICY_ALLOW_OBSOLETE_MSVCRT enabled)
set(VCPKG_POLICY_MISMATCHED_NUMBER_OF_BINARIES enabled)
set(VCPKG_FIXUP_MACHO_RPATH OFF)
set(VCPKG_FIXUP_ELF_RPATH OFF)

if(VCPKG_TARGET_IS_WINDOWS)
    vcpkg_download_distfile(
        SLVOICE_ARCHIVE
        URLS https://automated-builds-secondlife-com.s3.amazonaws.com/gh/secondlife/3p-slvoice/slvoice-4.10.0000.32327.5fc3fe7c.5942f08-windows64-5942f08.tar.zst
        FILENAME slvoice-win64.tar.zst
        SHA512 4bd5cd37e07f03e1e94f67189499cb8283cfa70804b6d83d5ff821fdc1cb6ecc7b6d3fdde5413ce161918a1d9b40eb7ce76c7c5fe45b068b0a3adee42f231dde
    )

    vcpkg_extract_source_archive(
        SLVOICE_DIR
        ARCHIVE ${SLVOICE_ARCHIVE}
        NO_REMOVE_ONE_LEVEL
    )

    file(INSTALL
        DIRECTORY "${SLVOICE_DIR}/bin/release/"
        DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}/windows64"
        FILES_MATCHING
        PATTERN "*.exe"
        PATTERN "*.pdb"
    )

    file(INSTALL
        DIRECTORY "${SLVOICE_DIR}/lib/release/"
        DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}/windows64"
        FILES_MATCHING
        PATTERN "*.dll"
        PATTERN "*.pdb"
    )

    vcpkg_install_copyright(FILE_LIST ${SLVOICE_DIR}/LICENSES/vivox_licenses.txt ${SLVOICE_DIR}/LICENSES/vivox_sdk_license.txt)
elseif(VCPKG_TARGET_IS_OSX)
    vcpkg_download_distfile(
        SLVOICE_ARCHIVE
        URLS https://automated-builds-secondlife-com.s3.amazonaws.com/gh/secondlife/3p-slvoice/slvoice-4.10.0000.32327.5fc3fe7c.5942f08-darwin64-5942f08.tar.zst
        FILENAME slvoice-darwin64.tar.zst
        SHA512 cc686f685d324ddeda86e68dca732adadb31dd6c6fa0ecc1d879f3321a6bb37561170da27d7d46f77a2609b1999384f67a2909ed626196e8732e7e32ca4a8ae5
    )

    vcpkg_extract_source_archive(
        SLVOICE_DIR
        ARCHIVE ${SLVOICE_ARCHIVE}
        NO_REMOVE_ONE_LEVEL
    )

    file(INSTALL
        DIRECTORY "${SLVOICE_DIR}/bin/release/"
        DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}/darwin64"
        FILES_MATCHING
        PATTERN "SLVoice"
    )

    file(INSTALL
        DIRECTORY "${SLVOICE_DIR}/lib/release/"
        DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}/darwin64"
        FILES_MATCHING
        PATTERN "*.dylib"
    )

    vcpkg_install_copyright(FILE_LIST ${SLVOICE_DIR}/LICENSES/vivox_licenses.txt ${SLVOICE_DIR}/LICENSES/vivox_sdk_license.txt)
elseif(VCPKG_TARGET_IS_LINUX)
    vcpkg_download_distfile(
        SLVOICE_ARCHIVE
        URLS http://automated-builds-secondlife-com.s3.amazonaws.com/ct2/613/1289/slvoice-3.2.0002.10426.500605-linux64-500605.tar.bz2
        FILENAME slvoice-linux.tar.zst
        SHA512 2941da523a5495fc56e841d2ef855d8eefaf74795e809adf09cc91924599ba38974963d937d9d529c153619c92e30a12aa24d7e4baa7d0a4cee859ba4a59996e
    )

    vcpkg_extract_source_archive(
        SLVOICE_DIR
        ARCHIVE ${SLVOICE_ARCHIVE}
        NO_REMOVE_ONE_LEVEL
    )

    file(INSTALL
        DIRECTORY "${SLVOICE_DIR}/lib/release/"
        DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}/linux"
        USE_SOURCE_PERMISSIONS
        FILES_MATCHING
        PATTERN "*.so*"
        PATTERN "SLVoice"
    )

    vcpkg_install_copyright(FILE_LIST ${SLVOICE_DIR}/LICENSES/slvoice.txt)
endif()
