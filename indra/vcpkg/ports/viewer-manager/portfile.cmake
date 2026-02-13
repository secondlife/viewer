set(VCPKG_POLICY_ALLOW_OBSOLETE_MSVCRT enabled)
set(VCPKG_POLICY_MISMATCHED_NUMBER_OF_BINARIES enabled)
set(VCPKG_FIXUP_MACHO_RPATH OFF)

if(VCPKG_TARGET_IS_WINDOWS)
    vcpkg_download_distfile(
        VMP_ARCHIVE
        URLS https://github.com/secondlife/viewer-manager/releases/download/v3.0-f14b5ec-D591/viewer_manager-3.0-f14b5ec-windows64-f14b5ec.tar.zst
        FILENAME vmp-win64.tar.zst
        SHA512 e9faccde36989b4bc177b3eca1f4c5d183d812038b2cf2022e716622474f9aa4c6d5fbbc6e2e02c75465590dcea7e34439b15aedb8eb378a7dd345d7ff7463cf
    )
elseif(VCPKG_TARGET_IS_OSX)
    vcpkg_download_distfile(
        VMP_ARCHIVE
        URLS https://github.com/secondlife/viewer-manager/releases/download/v3.0-f14b5ec-D591/viewer_manager-3.0-f14b5ec-darwin64-f14b5ec.tar.zst
        FILENAME vmp-darwin64.tar.zst
        SHA512 3b311bf3722493f8f2c6520356f7b299e50d90fd4f90eacf33281d2af9676c24ec288b0af795ad3c9cd86a64efdf25a06cfd61fb5e0c23fdcc7770c77ae8cd5e
    )
endif()

vcpkg_extract_source_archive(
    VMP_DIR
    ARCHIVE ${VMP_ARCHIVE}
    NO_REMOVE_ONE_LEVEL
)

file(INSTALL
    DIRECTORY "${VMP_DIR}/VMP/"
    DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}/"
    USE_SOURCE_PERMISSIONS
    FILES_MATCHING
    PATTERN "*.exe"
    PATTERN "SLVersionChecker"
)

vcpkg_install_copyright(FILE_LIST ${VMP_DIR}/LICENSE)
