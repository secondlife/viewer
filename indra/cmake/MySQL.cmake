# -*- cmake -*-
include(Linking)
include(Prebuilt)

use_prebuilt_binary(mysql)

if (LINUX)
  if (WORD_SIZE EQUAL 32 OR DEBIAN_VERSION STREQUAL "3.1")
    set(MYSQL_LIBRARIES mysqlclient)
    set(MYSQL_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/include)
  else (WORD_SIZE EQUAL 32 OR DEBIAN_VERSION STREQUAL "3.1")
    # Use the native MySQL library on a 64-bit system.
    set(MYSQL_FIND_QUIETLY ON)
    set(MYSQL_FIND_REQUIRED ON)
    include(FindMySQL)
  endif (WORD_SIZE EQUAL 32 OR DEBIAN_VERSION STREQUAL "3.1")
elseif (WINDOWS)
  set(MYSQL_LIBRARIES mysqlclient)
  set(MYSQL_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/include)
elseif (DARWIN)
  set(MYSQL_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/include)
  set(MYSQL_LIBRARIES
    optimized ${ARCH_PREBUILT_DIRS_RELEASE}/libmysqlclient.a
    debug ${ARCH_PREBUILT_DIRS_DEBUG}/libmysqlclient.a
    )
endif (LINUX)
