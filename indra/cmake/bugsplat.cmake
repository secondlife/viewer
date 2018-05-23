# BUGSPLAT can be set when launching the make using the argument -DBUGSPLAT:BOOL=ON
# When building using proprietary binaries though (i.e. having access to LL private servers),
# we always build with BUGSPLAT.
# Open source devs should use the -DBUGSPLAT:BOOL=ON then if they want to
# build with BugSplat, whether they are using USESYSTEMLIBS or not.
if (INSTALL_PROPRIETARY)
  set(BUGSPLAT ON CACHE BOOL "Using BugSplat crash reporting library.")
endif (INSTALL_PROPRIETARY)

if (BUGSPLAT)
  if (USESYSTEMLIBS)
    set(BUGSPLAT_FIND_QUIETLY ON)
    set(BUGSPLAT_FIND_REQUIRED ON)
    include(FindBUGSPLAT)
  else (USESYSTEMLIBS)
    include(Prebuilt)
    use_prebuilt_binary(bugsplat)
    if (WINDOWS)
      set(BUGSPLAT_LIBRARIES 
        ${ARCH_PREBUILT_DIRS_RELEASE}/bugsplat.lib
        )
    elseif (DARWIN)
      find_library(BUGSPLAT_LIBRARIES BugsplatMac
        PATHS "${ARCH_PREBUILT_DIRS_RELEASE}")
    else (WINDOWS)

    endif (WINDOWS)
    set(BUGSPLAT_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/include/bugsplat)
  endif (USESYSTEMLIBS)
endif (BUGSPLAT)
