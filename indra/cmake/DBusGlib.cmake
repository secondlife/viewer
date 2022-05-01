# -*- cmake -*-
include(Prebuilt)

add_library( ll::dbus INTERFACE IMPORTED)

# Only define this when using conan, lls prebuild is broken
if( USE_CONAN )
  target_compile_definitions( ll::dbus INTERFACE LL_DBUS_ENABLED )
endif()
use_conan_binary(dbus)

use_prebuilt_binary(dbus_glib)

