# -*- cmake -*-
include(Prebuilt)

add_library( ll::dbus INTERFACE IMPORTED)

if( LINUX )
  # Only define this when not using the prebuild 3ps, lls prebuild is broken
  if( NOT USE_AUTOBUILD_3P )
    target_compile_definitions( ll::dbus INTERFACE LL_DBUS_ENABLED )
  endif()
  use_system_binary(dbus)

  use_prebuilt_binary(dbus_glib)
endif()
