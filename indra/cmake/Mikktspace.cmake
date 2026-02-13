# -*- cmake -*-
include_guard()

include(Prebuilt)
include(Linking)

use_prebuilt_binary(mikktspace)

target_include_directories(ll::mikktspace SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include/)
