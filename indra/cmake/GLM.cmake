# -*- cmake -*-
include_guard()
include(Prebuilt)

add_library( ll::glm INTERFACE IMPORTED )

use_system_binary( glm )
use_prebuilt_binary(glm)
