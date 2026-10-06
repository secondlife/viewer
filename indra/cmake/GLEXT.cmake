# -*- cmake -*-
include(Prebuilt)

add_library(ll::glext INTERFACE IMPORTED)
use_system_binary(glext)
use_prebuilt_binary(glext)


