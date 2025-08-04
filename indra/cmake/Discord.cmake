include(Prebuilt)

add_library(ll::discord INTERFACE IMPORTED)
target_compile_definitions(ll::discord INTERFACE LL_DISCORD=1)

use_prebuilt_binary(discord)

target_include_directories(ll::discord SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include)
target_link_libraries(ll::discord INTERFACE discord_partner_sdk)
