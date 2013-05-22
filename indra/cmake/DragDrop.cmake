# -*- cmake -*-

set(OS_DRAG_DROP ON CACHE BOOL "Build the viewer with OS level drag and drop turned on or off")

if (OS_DRAG_DROP)

  if (WINDOWS)
    add_definitions(-DLL_OS_DRAGDROP_ENABLED=1)
  endif (WINDOWS)

  if (DARWIN)
    add_definitions(-DLL_OS_DRAGDROP_ENABLED=1)
  endif (DARWIN)

  if (LINUX)
    add_definitions(-DLL_OS_DRAGDROP_ENABLED=0)
  endif (LINUX)

endif (OS_DRAG_DROP)

