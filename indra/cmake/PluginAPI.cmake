# -*- cmake -*-

add_library( ll::pluginlibraries INTERFACE IMPORTED )

if (WINDOWS)
  target_link_libraries( ll::pluginlibraries INTERFACE
      wsock32
      ws2_32
      psapi
      netapi32
      advapi32
      user32
      )
endif (WINDOWS)


