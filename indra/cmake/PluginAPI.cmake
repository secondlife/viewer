# -*- cmake -*-

create_target( ll::pluginlibraries)

if (WINDOWS)
  set_target_libraries( ll::pluginlibraries
      wsock32
      ws2_32
      psapi
      netapi32
      advapi32
      user32
      )
endif (WINDOWS)


