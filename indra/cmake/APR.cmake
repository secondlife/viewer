include(Linking)
include(Prebuilt)

include_guard()

add_library( ll::apr INTERFACE IMPORTED )

use_system_binary( apr apr-util )
use_prebuilt_binary(apr_suite)

if (WINDOWS)
  target_link_libraries( ll::apr INTERFACE
          ${ARCH_PREBUILT_DIRS_RELEASE}/apr-1.lib
          ${ARCH_PREBUILT_DIRS_RELEASE}/aprutil-1.lib
          )
  target_compile_definitions( ll::apr INTERFACE APR_DECLARE_STATIC=1 APU_DECLARE_STATIC=1 API_DECLARE_STATIC=1)
elseif (DARWIN)
  target_link_libraries( ll::apr INTERFACE
          ${ARCH_PREBUILT_DIRS_RELEASE}/libapr-1.a
          ${ARCH_PREBUILT_DIRS_RELEASE}/libaprutil-1.a
          iconv
          )
else()
  target_link_libraries( ll::apr INTERFACE
          ${ARCH_PREBUILT_DIRS_RELEASE}/libapr-1.a
          ${ARCH_PREBUILT_DIRS_RELEASE}/libaprutil-1.a
          rt
          )
endif ()
target_include_directories( ll::apr SYSTEM INTERFACE  ${LIBS_PREBUILT_DIR}/include/apr-1 )
