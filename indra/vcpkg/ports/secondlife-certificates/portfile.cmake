set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)

vcpkg_download_distfile(
    LLCA_ARCHIVE
    URLS "https://github.com/secondlife/llca/releases/download/v202407221723.0-a0fd5b9/llca-202407221423.0-common-10042698865.tar.zst"
    FILENAME llca.tar.zst
    SHA512 8abfb35394a4c32ad0c6d0b042c0bad44be84f60cab5c170fa8a965b45eb5baf21559c4840be41422cf95ad26803f3314fc1a2a0e3391460cc29de32c051d246
)

vcpkg_extract_source_archive(LLCA_DIR ARCHIVE ${LLCA_ARCHIVE} NO_REMOVE_ONE_LEVEL)

file(INSTALL "${LLCA_DIR}/ca-bundle.crt" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
message(STATUS "Importing certstore done")

vcpkg_install_copyright(FILE_LIST "${LLCA_DIR}/LICENSES/ca-license.txt")

# Below is the routine required to generate this package using vcpkg
# vcpkg_find_acquire_program(PERL)
# get_filename_component(PERL_EXE_PATH ${PERL} DIRECTORY)
# set(ENV{PATH} "$ENV{PATH};${PERL_EXE_PATH}")

# vcpkg_find_acquire_program(PERL)

# vcpkg_download_distfile(
#     LLCA_ARCHIVE
#     URLS "https://raw.githubusercontent.com/mozilla-firefox/firefox/refs/tags/FIREFOX_147_0_1_BUILD1/security/nss/lib/ckfw/builtins/certdata.txt"
#     FILENAME certdata.txt
#     SHA512 64fb1e7912fa5d2379728038563635161983ee7a0ca4a79c6a61b71d808d28a93ef768e5b15c8b408ec62c0e6dfafd449de0d3eb14468cd34b615265f2ed7a91
# )

# file(COPY ${LLCA_ARCHIVE} DESTINATION "${CURRENT_BUILDTREES_DIR}")
# file(COPY "${CMAKE_CURRENT_LIST_DIR}/mk-ca-bundle.pl" DESTINATION "${CURRENT_BUILDTREES_DIR}")

# vcpkg_execute_required_process(
#     COMMAND ${PERL} ${CURRENT_BUILDTREES_DIR}/mk-ca-bundle.pl -n ${CURRENT_BUILDTREES_DIR}/ca-bundle.crt
#     WORKING_DIRECTORY ${CURRENT_BUILDTREES_DIR}
#     LOGNAME ca-bundle
# )

# file(INSTALL "${CURRENT_BUILDTREES_DIR}/ca-bundle.crt" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
# message(STATUS "Importing certstore done")

# vcpkg_install_copyright(FILE_LIST ${CMAKE_CURRENT_LIST_DIR}/ca-license.txt)
