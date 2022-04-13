include(Linking)
include(Prebuilt)

include_guard()

create_target( ll::apr)

use_prebuilt_binary(apr_suite)
if (WINDOWS)
  if (LLCOMMON_LINK_SHARED)
    set(APR_selector "lib")
  else (LLCOMMON_LINK_SHARED)
    set(APR_selector "")
  endif (LLCOMMON_LINK_SHARED)
  set_target_libraries( ll::apr
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

  set_target_libraries( ll::apr
          libapr-1.${APR_selector}
          libaprutil-1.${APRUTIL_selector}
          iconv
          )
else (WINDOWS)
  set_target_libraries( ll::apr
          apr-1
          aprutil-1
          iconv
          uuid
          rt
          )
endif (WINDOWS)
set_target_include_dirs(  ll::apr  ${LIBS_PREBUILT_DIR}/include/apr-1 )
