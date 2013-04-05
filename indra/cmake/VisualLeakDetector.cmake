# -*- cmake -*-

set(INCLUDE_VLD_CMAKE OFF CACHE BOOL "Build the Windows viewer with Visual Leak Detector turned on or off")

if (INCLUDE_VLD_CMAKE)

  if (WINDOWS)
    add_definitions(-DINCLUDE_VLD=1)
  endif (WINDOWS)

endif (INCLUDE_VLD_CMAKE)

