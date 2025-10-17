include(Linking)
include(Prebuilt)

include_guard()

add_library( ll::apr INTERFACE IMPORTED )

use_system_binary( apr apr-util )
use_prebuilt_binary(apr_suite)

if (WINDOWS)
  target_compile_definitions(ll::apr INTERFACE APR_DECLARE_STATIC=1 APU_DECLARE_STATIC=1 API_DECLARE_STATIC=1)
endif ()

find_library(APR_LIBRARY
    NAMES
    apr-1.lib
    libapr-1.a
    PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

find_library(APRUTIL_LIBRARY
    NAMES
    aprutil-1.lib
    libaprutil-1.a
    PATHS "${ARCH_PREBUILT_DIRS_RELEASE}" REQUIRED NO_DEFAULT_PATH)

target_link_libraries(ll::apr INTERFACE ${APR_LIBRARY} ${APRUTIL_LIBRARY})

if(DARWIN)
  target_link_libraries(ll::apr INTERFACE iconv)
endif()

target_include_directories(ll::apr SYSTEM INTERFACE ${LIBS_PREBUILT_DIR}/include/apr-1)
