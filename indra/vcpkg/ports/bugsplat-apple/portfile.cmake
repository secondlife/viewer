set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)
set(VCPKG_FIXUP_MACHO_RPATH OFF)

# fetch 3p-bugsplat for upload scripts
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO secondlife/3p-bugsplat
    REF v1.2.6-a475cbb
    SHA512 408ff7160c7596e8e005b9e9e85f87b727fc5068c0a7966643ea157fdd7a3ef24a402f1757096ac9078016708281d8727c42e5b8ed067c0c9577a40f6dbce31a
    HEAD_REF main
)

vcpkg_download_distfile(
    BUGSPLAT_ARCHIVE
    URLS https://github.com/BugSplat-Git/bugsplat-apple/releases/download/v2.0.0/BugSplat.xcframework.zip
    FILENAME dullahan-windows64.tar.zst
    SHA512 7934435679d021404b3929749008ba6d42725136b1eb156f7e45a915d9c3a728759edf60875e3baa02e54c2a7c90f7a84646ec03b131b500b2213dfed60e08cb
)

vcpkg_extract_source_archive(
    BUGSPLAT_DIR
    ARCHIVE ${BUGSPLAT_ARCHIVE}
    NO_REMOVE_ONE_LEVEL
)

file(MAKE_DIRECTORY "${CURRENT_PACKAGES_DIR}/lib")
vcpkg_execute_required_process(
    COMMAND cp -a "${BUGSPLAT_DIR}/BugSplat.xcframework/macos-arm64_x86_64/BugSplatMac.framework" "${CURRENT_PACKAGES_DIR}/lib/BugSplatMac.framework"
    WORKING_DIRECTORY ${CURRENT_BUILDTREES_DIR}
    LOGNAME build-${TARGET_TRIPLET}-dbg
)

vcpkg_execute_required_process(
    COMMAND cp -a "${BUGSPLAT_DIR}/BugSplat.xcframework/macos-arm64_x86_64/dSYMs/BugSplatMac.framework.dSYM" "${CURRENT_PACKAGES_DIR}/lib/BugSplatMac.framework.dSYM"
    WORKING_DIRECTORY ${CURRENT_BUILDTREES_DIR}
    LOGNAME build-${TARGET_TRIPLET}-dbg
)

file(INSTALL "${SOURCE_PATH}/upload-archive.sh" DESTINATION "${CURRENT_PACKAGES_DIR}/tools/upload-extensions/")
file(INSTALL "${SOURCE_PATH}/upload-mac-symbols.sh" DESTINATION "${CURRENT_PACKAGES_DIR}/tools/upload-extensions/")

file(INSTALL "${SOURCE_PATH}/BugSplat/BUGSPLAT_LICENSE.txt" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
