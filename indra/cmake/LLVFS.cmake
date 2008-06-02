# -*- cmake -*-

set(LLVFS_INCLUDE_DIRS
    ${LIBS_OPEN_DIR}/llvfs
    )

set(LLVFS_LIBRARIES llvfs)

if (DARWIN)
  include(CMakeFindFrameworks)
  find_library(CARBON_LIBRARY Carbon)
  list(APPEND LLVFS_LIBRARIES ${CARBON_LIBRARY})
endif (DARWIN)
