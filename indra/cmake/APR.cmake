include(Linking)
include(Prebuilt)

if( TARGET apr::apr )
  return()
endif()

create_target( apr::apr)

if (USESYSTEMLIBS)
  include(FindAPR)
else (USESYSTEMLIBS)
  use_prebuilt_binary(apr_suite)
  if (WINDOWS)
    if (LLCOMMON_LINK_SHARED)
      set(APR_selector "lib")
    else (LLCOMMON_LINK_SHARED)
      set(APR_selector "")
    endif (LLCOMMON_LINK_SHARED)
    set_target_libraries( apr::apr
            ${ARCH_PREBUILT_DIRS_RELEASE}/${APR_selector}apr-1.lib
            ${ARCH_PREBUILT_DIRS_RELEASE}/${APR_selector}apriconv-1.lib
            ${ARCH_PREBUILT_DIRS_RELEASE}/${APR_selector}aprutil-1.lib
            )
  elseif (DARWIN)
    if (LLCOMMON_LINK_SHARED)
      set(APR_selector     "0.dylib")
      set(APRUTIL_selector "0.dylib")
    else (LLCOMMON_LINK_SHARED)
      set(APR_selector     "a")
      set(APRUTIL_selector "a")
    endif (LLCOMMON_LINK_SHARED)

    set_target_libraries( apr::apr
            libapr-1.${APR_selector}
            libaprutil-1.${APRUTIL_selector}
            iconv
            )
  else (WINDOWS)
    set_target_libraries( apr::apr
            apr-1
            aprutil-1
            iconv
            uuid
            rt
            )
  endif (WINDOWS)
  set_target_include_dirs(  apr::apr  ${LIBS_PREBUILT_DIR}/include/apr-1 )
endif (USESYSTEMLIBS)
