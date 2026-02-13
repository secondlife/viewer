vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO milesj/emojibase
    REF emojibase-data@${VERSION}
    SHA512 59e51c16f6580085b4ef80ff79ec164ebc9c982f302ba7677daf65097ceed78cfad17cf865d0e3194aa2115733112eb8d09c6b4b2397cbdb993d90c90c9a4826
    HEAD_REF master
)

vcpkg_find_acquire_program(PYTHON3)

vcpkg_execute_required_process(
    COMMAND ${PYTHON3} "${CURRENT_PORT_DIR}/gen_emoji_characters.py" "${SOURCE_PATH}/packages/data" "${CURRENT_PACKAGES_DIR}/share/${PORT}/xui"
    WORKING_DIRECTORY "${CURRENT_BUILDTREES_DIR}"
    LOGNAME "build-${TARGET_TRIPLET}-dbg")

vcpkg_install_copyright(FILE_LIST ${SOURCE_PATH}/LICENSE)
