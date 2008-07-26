# -*- cmake -*-
include(Linking)
include(Prebuilt)
# We don't prebuild our own MySQL client library.

use_prebuilt_binary(mysql)

set(MYSQL_FIND_QUIETLY ON)
set(MYSQL_FIND_REQUIRED ON)

if (WINDOWS)
  set(MYSQL_LIBRARIES mysqlclient)
  set(MYSQL_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/${LL_ARCH_DIR}/include)
elseif (DARWIN)
  set(MYSQL_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/${LL_ARCH_DIR}/include)
  set(MYSQL_LIBRARIES
    optimized ${LIBS_PREBUILT_DIRS_RELEASE}/libmysqlclient.a
    debug ${LIBS_PREBUILT_DIRS_DEBUG}/libmysqlclient.a
    )
else (WINDOWS)
    set(MYSQL_FIND_REQUIRED)
    include(FindMySQL)
endif (WINDOWS)
