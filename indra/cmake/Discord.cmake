include(Prebuilt)

add_library(ll::discord_sdk INTERFACE IMPORTED)
target_compile_definitions(ll::discord_sdk INTERFACE LL_DISCORD=1)

use_prebuilt_binary(discord_sdk)

target_include_directories(ll::discord_sdk SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include/discord_sdk)
target_link_libraries(ll::discord_sdk INTERFACE discord_partner_sdk)
