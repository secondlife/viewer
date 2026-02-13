vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO AlchemyViewer/libndofdev
    REF "v${VERSION}"
    SHA512 113339c2f4a8666284bd297148f46e4c73bca4e9f5e38644805add674377a4eec99c0b91b625fd552d0906a1ca6da0a29fb5a285e36f4e3029ce32ee029dbb93
    HEAD_REF main
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
)
vcpkg_cmake_install()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

