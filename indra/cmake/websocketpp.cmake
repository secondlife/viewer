# -*- cmake -*-
include(Prebuilt)

add_library( ll::websocketpp INTERFACE IMPORTED )

use_system_binary( websocketpp )
use_prebuilt_binary(websocketpp)
