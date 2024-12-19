# -*- cmake -*-
if (SQLite3_CMAKE_INCLUDED)
  return()
endif (SQLite3_CMAKE_INCLUDED)
set (SQLite3_CMAKE_INCLUDED TRUE)

include(Prebuilt)

use_prebuilt_binary(libsqlite3)

set(SQLite3_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include)
if (LINUX)
  set(SQLite3_LIBRARIES ${ARCH_PREBUILT_DIRS_RELEASE}/libsqlite3.a)
elseif (WINDOWS)
  set(SQLite3_LIBRARIES ${ARCH_PREBUILT_DIRS_RELEASE}/sqlite3.lib)
elseif (DARWIN)
  set(SQLite3_LIBRARIES ${ARCH_PREBUILT_DIRS_RELEASE}/libsqlite3.a)
endif (LINUX)

include_directories(SYSTEM SQLite3_INCLUDE_DIRS)
