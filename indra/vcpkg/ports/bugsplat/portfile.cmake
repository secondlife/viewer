set(VCPKG_POLICY_DLLS_WITHOUT_EXPORTS enabled)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO secondlife/3p-bugsplat
    REF v1.2.6-a475cbb
    SHA512 408ff7160c7596e8e005b9e9e85f87b727fc5068c0a7966643ea157fdd7a3ef24a402f1757096ac9078016708281d8727c42e5b8ed067c0c9577a40f6dbce31a
    HEAD_REF main
)

file(INSTALL
    DIRECTORY "${SOURCE_PATH}/BugSplat/BugSplat/inc/"
    DESTINATION "${CURRENT_PACKAGES_DIR}/include/bugsplat/"
    FILES_MATCHING
    PATTERN "*.h"
)

file(INSTALL
    DIRECTORY "${SOURCE_PATH}/BugSplat/BugSplat/x64/Release/"
    DESTINATION "${CURRENT_PACKAGES_DIR}/lib/"
    FILES_MATCHING
    PATTERN "*.lib"
)

file(INSTALL
    DIRECTORY "${SOURCE_PATH}/BugSplat/BugSplat/x64/Release/"
    DESTINATION "${CURRENT_PACKAGES_DIR}/bin/"
    FILES_MATCHING
    PATTERN "*.dll"
)

file(INSTALL
    DIRECTORY "${SOURCE_PATH}/BugSplat/BugSplat/x64/Release/"
    DESTINATION "${CURRENT_PACKAGES_DIR}/bin/"
    FILES_MATCHING
    PATTERN "*.pdb"
)

file(INSTALL
    DIRECTORY "${SOURCE_PATH}/BugSplat/BugSplat/x64/Release/"
    DESTINATION "${CURRENT_PACKAGES_DIR}/tools/"
    FILES_MATCHING
    PATTERN "*.exe"
)

if(NOT (VCPKG_BUILD_TYPE STREQUAL "release"))
    file(INSTALL
        DIRECTORY "${SOURCE_PATH}/BugSplat/BugSplat/x64/Debug/"
        DESTINATION "${CURRENT_PACKAGES_DIR}/debug/lib/"
        FILES_MATCHING
        PATTERN "*.lib"
    )

    file(INSTALL
        DIRECTORY "${SOURCE_PATH}/BugSplat/BugSplat/x64/Debug/"
        DESTINATION "${CURRENT_PACKAGES_DIR}/debug/bin/"
        FILES_MATCHING
        PATTERN "*.dll"
    )

    file(INSTALL
        DIRECTORY "${SOURCE_PATH}/BugSplat/BugSplat/x64/Debug/"
        DESTINATION "${CURRENT_PACKAGES_DIR}/debug/bin/"
        FILES_MATCHING
        PATTERN "*.pdb"
    )

    file(INSTALL
        DIRECTORY "${SOURCE_PATH}/BugSplat/BugSplat/x64/Debug/"
        DESTINATION "${CURRENT_PACKAGES_DIR}/debug/tools/"
        FILES_MATCHING
        PATTERN "*.exe"
    )
endif()


file(INSTALL "${SOURCE_PATH}/BugSplat/Tools/symbol-upload-windows.exe" DESTINATION "${CURRENT_PACKAGES_DIR}/tools")
file(INSTALL "${SOURCE_PATH}/upload-windows-symbols.sh" DESTINATION "${CURRENT_PACKAGES_DIR}/tools/upload-extensions/")
file(INSTALL "${SOURCE_PATH}/SendPdbs.bat" DESTINATION "${CURRENT_PACKAGES_DIR}/tools/upload-extensions/")

file(INSTALL "${SOURCE_PATH}/BugSplat/BUGSPLAT_LICENSE.txt" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
