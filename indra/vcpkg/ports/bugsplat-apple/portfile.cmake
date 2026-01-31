set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)
set(VCPKG_FIXUP_MACHO_RPATH OFF)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO secondlife/3p-bugsplat
    REF v1.2.6-a475cbb
    SHA512 408ff7160c7596e8e005b9e9e85f87b727fc5068c0a7966643ea157fdd7a3ef24a402f1757096ac9078016708281d8727c42e5b8ed067c0c9577a40f6dbce31a
    HEAD_REF main
)

vcpkg_execute_required_process(
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${SOURCE_PATH}/BugSpaltxcframework/BugSplat.xcframework/macos-arm64_x86_64/BugsplatMac.framework" "${CURRENT_PACKAGES_DIR}/lib/BugsplatMac.framework"
    WORKING_DIRECTORY ${CURRENT_BUILDTREES_DIR}
    LOGNAME build-${TARGET_TRIPLET}-dbg
)

vcpkg_execute_required_process(
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${SOURCE_PATH}/BugSpaltxcframework/CrashReporter.xcframework/macos-arm64_x86_64/CrashReporter.framework" "${CURRENT_PACKAGES_DIR}/lib/CrashReporter.framework"
    WORKING_DIRECTORY ${CURRENT_BUILDTREES_DIR}
    LOGNAME build-${TARGET_TRIPLET}-dbg
)

vcpkg_execute_required_process(
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${SOURCE_PATH}/BugSpaltxcframework/HockeySDK.xcframework/macos-arm64_x86_64/HockeySDK.framework" "${CURRENT_PACKAGES_DIR}/lib/HockeySDK.framework"
    WORKING_DIRECTORY ${CURRENT_BUILDTREES_DIR}
    LOGNAME build-${TARGET_TRIPLET}-dbg
)

file(INSTALL "${SOURCE_PATH}/upload-archive.sh" DESTINATION "${CURRENT_PACKAGES_DIR}/tools/upload-extensions/")
file(INSTALL "${SOURCE_PATH}/upload-mac-symbols.sh" DESTINATION "${CURRENT_PACKAGES_DIR}/tools/upload-extensions/")

file(INSTALL "${SOURCE_PATH}/BugSplat/BUGSPLAT_LICENSE.txt" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
