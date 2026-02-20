set(VCPKG_POLICY_ALLOW_EMPTY_FOLDERS enabled)
set(VCPKG_POLICY_ALLOW_OBSOLETE_MSVCRT enabled)
set(VCPKG_POLICY_MISMATCHED_NUMBER_OF_BINARIES enabled)

if(VCPKG_TARGET_IS_WINDOWS)
    set(WEBRTC_LIBNAME "webrtc.lib")

    vcpkg_download_distfile(
        WEBRTC_ARCHIVE
        URLS https://github.com/secondlife/3p-webrtc-build/releases/download/m137.7151.04.21/webrtc-m137.7151.04.21.18609120431-windows64-18609120431.tar.zst
        FILENAME webrtc-windows64.tar.zst
        SHA512 810c6bfaec734c48d799c603aed5843ba8600fb6221d08437f73c382af5d86b46e75ea3957ae7b242032756008c202ea73cfc2a0c0070227bd77b33bc48a56d4
    )
elseif(VCPKG_TARGET_IS_OSX)
    set(WEBRTC_LIBNAME "libwebrtc.a")

    vcpkg_download_distfile(
        WEBRTC_ARCHIVE
        URLS https://github.com/secondlife/3p-webrtc-build/releases/download/m137.7151.04.21/webrtc-m137.7151.04.21.18609120431-darwin64-18609120431.tar.zst
        FILENAME webrtc-osx.tar.zst
        SHA512 0c037b32ed5fedff0e2afe480f08983afe7d72dbfd2fa01e6749678e14ae94bff1ef402a0246feb5977eaa43467748fc729c1ab9c410465a03679b9dcac5f38a
    )
elseif(VCPKG_TARGET_IS_LINUX)
    set(WEBRTC_LIBNAME "libwebrtc.a")

    vcpkg_download_distfile(
        WEBRTC_ARCHIVE
        URLS https://github.com/secondlife/3p-webrtc-build/releases/download/m137.7151.04.21/webrtc-m137.7151.04.21.18609120431-linux64-18609120431.tar.zst
        FILENAME webrtc-linux64.tar.zst
        SHA512 44f52c8c0fc3efe64428c90da3f4ac322ba3c107a5ab42851b59b27e324d984cbeed7aefa8e1ff83b3cf5e3092ca86214ed4bacc0c13ea2836f6e53ce6f38ecd
    )
endif()

vcpkg_extract_source_archive(
    WEBRTC_DIR
    ARCHIVE ${WEBRTC_ARCHIVE}
    NO_REMOVE_ONE_LEVEL
)

file(MAKE_DIRECTORY "${CURRENT_PACKAGES_DIR}/include/")
file(MAKE_DIRECTORY "${CURRENT_PACKAGES_DIR}/lib/")
file(RENAME "${WEBRTC_DIR}/include/webrtc/" "${CURRENT_PACKAGES_DIR}/include/webrtc/")
if (VCPKG_TARGET_IS_LINUX)
    file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/include/webrtc/build/linux/debian_bullseye_i386-sysroot")
    file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/include/webrtc/build/linux/debian_bullseye_amd64-sysroot")
endif()

file(RENAME "${WEBRTC_DIR}/lib/release/${WEBRTC_LIBNAME}" "${CURRENT_PACKAGES_DIR}/lib/${WEBRTC_LIBNAME}")

vcpkg_install_copyright(FILE_LIST ${WEBRTC_DIR}/LICENSES/webrtc-license.txt)
