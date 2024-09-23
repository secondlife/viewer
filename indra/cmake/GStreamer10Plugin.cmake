# -*- cmake -*-

include_guard()

include(Prebuilt)
include(GLIB)

add_library( ll::gstreamer10 INTERFACE IMPORTED )

if (LINUX)
  include(FindPkgConfig)

  pkg_check_modules(GSTREAMER10 REQUIRED gstreamer-1.0)
  pkg_check_modules(GSTREAMER10_PLUGINS_BASE REQUIRED gstreamer-plugins-base-1.0)

  target_include_directories( ll::gstreamer10 SYSTEM INTERFACE ${GSTREAMER10_INCLUDE_DIRS})
  target_link_libraries( ll::gstreamer10 INTERFACE  ll::glib_headers)

endif ()

if (GSTREAMER10_FOUND AND GSTREAMER10_PLUGINS_BASE_FOUND)
  set(GSTREAMER10 ON CACHE BOOL "Build with GStreamer-1.0 streaming media support.")
endif (GSTREAMER10_FOUND AND GSTREAMER10_PLUGINS_BASE_FOUND)

if (GSTREAMER10)
  add_definitions(-DLL_GSTREAMER10_ENABLED=1)
endif (GSTREAMER10)
