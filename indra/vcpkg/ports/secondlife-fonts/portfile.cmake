vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO secondlife/3p-viewer-fonts
    REF "v${VERSION}"
    SHA512 d78571fe96a6b8bb102c07dc7efea4c61d01e27891b94465d741f169ccb107633c7764aa92101cb01be50964d28845c99b67c103952a0a758cc55f88d07a6537
    HEAD_REF main
)

file(INSTALL
    DIRECTORY "${SOURCE_PATH}/src/"
    DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}/fonts"
    FILES_MATCHING
    PATTERN "*.ttf"
    PATTERN "*.otf"
)

vcpkg_install_copyright(FILE_LIST ${SOURCE_PATH}/src/DejaVu-license.txt ${SOURCE_PATH}/src/Twemoji-Artwork-CC-BY-license.txt ${SOURCE_PATH}/src/Twemoji-MIT-license.txt)
