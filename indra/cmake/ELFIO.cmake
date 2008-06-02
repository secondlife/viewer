# -*- cmake -*-

set(ELFIO_FIND_QUIETLY ON)

if (STANDALONE)
  include(FindELFIO)
elseif (LINUX)
  set(ELFIO_LIBRARIES ELFIO)
  set(ELFIO_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/include)
endif (STANDALONE)

if (ELFIO_FOUND)
  add_definitions(-DLL_ELFBIN=1)
else (ELFIO_FOUND)
  set(ELFIO_INCLUDE_DIR "")
endif (ELFIO_FOUND)
