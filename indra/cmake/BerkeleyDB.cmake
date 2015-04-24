# -*- cmake -*-
include(Prebuilt)
set(DB_FIND_QUIETLY ON)
set(DB_FIND_REQUIRED ON)

if (USESYSTEMLIBS)
  include(FindBerkeleyDB)
else (USESYSTEMLIBS)
  if (LINUX)
    # Need to add dependency pthread explicitely to support ld.gold.
    use_prebuilt_binary(db)
    set(DB_LIBRARIES db-5.1 pthread)
  else (LINUX)
    set(DB_LIBRARIES db-4.2)
  endif (LINUX)
  set(DB_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include)
endif (USESYSTEMLIBS)
