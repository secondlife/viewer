# -*- cmake -*-

if (WINDOWS)
  set(PLUGIN_API_WINDOWS_LIBRARIES
      wsock32
      ws2_32
      psapi
      netapi32
      advapi32
      user32
      )
else (WINDOWS)
  set(PLUGIN_API_WINDOWS_LIBRARIES "")
endif (WINDOWS)


