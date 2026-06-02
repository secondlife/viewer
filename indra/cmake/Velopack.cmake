# -*- cmake -*-
# Velopack installer and update framework integration
# https://velopack.io/

include_guard()

# USE_VELOPACK controls whether to use Velopack for installer packaging (instead of NSIS/DMG)
option(USE_VELOPACK "Use Velopack for installer packaging" OFF)

if (WINDOWS)
    include(Prebuilt)
    use_prebuilt_binary(velopack)

    add_library(ll::velopack INTERFACE IMPORTED)

    target_include_directories(ll::velopack SYSTEM INTERFACE
        ${LIBS_PREBUILT_DIR}/include/velopack
    )

    target_link_libraries(ll::velopack INTERFACE
        ${ARCH_PREBUILT_DIRS_RELEASE}/velopack_libc.lib
    )

    # Windows system libraries required by Velopack
    target_link_libraries(ll::velopack INTERFACE
        winhttp
        ole32
        shell32
        shlwapi
        version
        userenv
        ws2_32
        bcrypt
        ntdll
    )

    target_compile_definitions(ll::velopack INTERFACE LL_VELOPACK=1)

elseif (DARWIN)
    include(Prebuilt)
    use_prebuilt_binary(velopack)

    add_library(ll::velopack INTERFACE IMPORTED)

    target_include_directories(ll::velopack SYSTEM INTERFACE
        ${LIBS_PREBUILT_DIR}/include/velopack
    )

    target_link_libraries(ll::velopack INTERFACE
        ${ARCH_PREBUILT_DIRS_RELEASE}/libvelopack_libc.a
    )

    # macOS system frameworks required by Velopack (Rust static library dependencies)
    target_link_libraries(ll::velopack INTERFACE
        "-framework Foundation"
        "-framework Security"
        "-framework SystemConfiguration"
        "-framework AppKit"
        "-framework CoreFoundation"
        "-framework CoreServices"
        "-framework IOKit"
        "-liconv"
        "-lresolv"
    )

    target_compile_definitions(ll::velopack INTERFACE LL_VELOPACK=1)

endif()
