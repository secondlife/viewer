vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO RyeMutt/3p-open-libndofdev
    REF "v${VERSION}"
    SHA512 e5676a36a1b16f18649221104ec9326e7e37e547bfef23a96bebb2af9fe5e9794697600924ca355d3fb98ec2d4e0e58f432d7690aa42bf7e7394e90eea0daa34
    HEAD_REF main
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}/libndofdev"
)
vcpkg_cmake_install()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/libndofdev/LICENSES/libndofdev.txt")

