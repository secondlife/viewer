include(Prebuilt)

set(BUGSPLAT_FIND_QUIETLY ON)
set(BUGSPLAT_FIND_REQUIRED ON)

if (USESYSTEMLIBS)
  include(FindBUGSPLAT)
else (USESYSTEMLIBS)
  use_prebuilt_binary(bugsplat)
  if (WINDOWS)
    set(BUGSPLAT_LIBRARIES 
      ${ARCH_PREBUILT_DIRS_RELEASE}/bugsplat.lib
      )
  elseif (DARWIN)

  else (WINDOWS)

  endif (WINDOWS)
  set(BUGSPLAT_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/include/bugsplat)
endif (USESYSTEMLIBS)
