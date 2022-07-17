include(Linking)
include(Prebuilt)

include_guard()

add_library( ll::apr INTERFACE IMPORTED )

use_system_binary( apr apr-util )
use_prebuilt_binary(apr_suite)

if (WINDOWS)
  if (LLCOMMON_LINK_SHARED)
    set(APR_selector "lib")
  else (LLCOMMON_LINK_SHARED)
    set(APR_selector "")
  endif (LLCOMMON_LINK_SHARED)
  target_link_libraries( ll::apr INTERFACE
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

  target_link_libraries( ll::apr INTERFACE
          libapr-1.${APR_selector}
          libaprutil-1.${APRUTIL_selector}
          iconv
          )
else (WINDOWS)
  target_link_libraries( ll::apr INTERFACE
          apr-1
          aprutil-1
          iconv
          uuid
          rt
          )
endif (WINDOWS)
target_include_directories( ll::apr SYSTEM INTERFACE  ${LIBS_PREBUILT_DIR}/include/apr-1 )
