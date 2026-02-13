# vcpkg_from_github(
#     OUT_SOURCE_PATH SOURCE_PATH
#     REPO secondlife/3p-dictionaries
#     REF "v${VERSION}"
#     SHA512 0
#     HEAD_REF main
# )

vcpkg_download_distfile(
    DICTIONARIES_ARCHIVE
    URLS https://github.com/secondlife/3p-dictionaries/releases/download/v1-a01bb6c/dictionaries-1.a01bb6c-common-a01bb6c.tar.zst
    FILENAME dictionaries.tar.zst
    SHA512 95c0f27727ea7b9e6af3f2fd25e560d93edd678fc6104ca74c0765ef9ae957cceacf029d159b1eb25571fdde0a721b956a59483e1820d1357e42a5c94a72ae86
)

vcpkg_extract_source_archive(
    DICTIONARIES_DIR
    ARCHIVE ${DICTIONARIES_ARCHIVE}
    NO_REMOVE_ONE_LEVEL
)

file(INSTALL
    DIRECTORY "${DICTIONARIES_DIR}/dictionaries"
    DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}/dictionaries"
    FILES_MATCHING
    PATTERN "*.xml"
    PATTERN "*.aff"
    PATTERN "*.dic"
)

vcpkg_install_copyright(FILE_LIST
    "${DICTIONARIES_DIR}/LICENSES/dictionaries.txt"
    "${DICTIONARIES_DIR}/LICENSES/en_gb-dictionary-license.txt"
    "${DICTIONARIES_DIR}/LICENSES/en_us-dictionary-license.txt"
    "${DICTIONARIES_DIR}/LICENSES/es_es-dictionary-license.txt"
    "${DICTIONARIES_DIR}/LICENSES/pt_br-dictionary-license.txt"
)
