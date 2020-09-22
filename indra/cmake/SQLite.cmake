# -*- cmake -*-
include(Prebuilt)

if (USESYSTEMLIBS)
  include(FindPkgConfig)
  pkg_check_modules(SQLITE REQUIRED sqlite3)
else (USESYSTEMLIBS)
  use_prebuilt_binary(sqlite)
  set(SQLITE_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/include/sqlite/)
  set(SQLITE_LIBRARIES sqlite)
endif (USESYSTEMLIBS)
