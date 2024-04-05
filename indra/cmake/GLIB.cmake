include_guard()

include(Prebuilt)

add_library( ll::glib INTERFACE IMPORTED )
add_library( ll::glib_headers INTERFACE IMPORTED )
add_library( ll::gio INTERFACE IMPORTED )

if( LINUX )
  find_package(PkgConfig REQUIRED)
  pkg_search_module(GLIB REQUIRED glib-2.0)
  pkg_search_module(GIO REQUIRED gio-2.0)

  target_include_directories( ll::glib SYSTEM INTERFACE ${GLIB_INCLUDE_DIRS}  )
  target_link_libraries( ll::glib INTERFACE ${GLIB_LDFLAGS} )
  target_compile_definitions( ll::glib INTERFACE -DLL_GLIB=1)

  target_include_directories( ll::glib_headers SYSTEM INTERFACE ${GLIB_INCLUDE_DIRS}  )
  target_compile_definitions( ll::glib_headers INTERFACE -DLL_GLIB=1)

  target_link_libraries( ll::gio INTERFACE ${GIO_LDFLAGS} )
endif()
