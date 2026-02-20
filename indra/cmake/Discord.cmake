include_guard()

add_library(ll::discord_sdk INTERFACE IMPORTED)

if(USE_DISCORD)
    target_compile_definitions(ll::discord_sdk INTERFACE LL_DISCORD=1)

    find_library(DISCORD_SDK_LIBRARY
        NAMES
        discord_partner_sdk
        discord_partner_sdk.lib
        libdiscord_partner_sdk.so
        libdiscord_partner_sdk.dylib
        PATHS "${ARCH_PREBUILT_DIRS_ARCH_RELEASE}" "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

    target_include_directories(ll::discord_sdk SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include/discord_sdk)
    target_link_libraries(ll::discord_sdk INTERFACE ${DISCORD_SDK_LIBRARY})
endif()
