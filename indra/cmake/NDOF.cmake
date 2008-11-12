# -*- cmake -*-
include(Prebuilt)

use_prebuilt_binary(ndofdev)

if (WINDOWS OR DARWIN OR LINUX)
  add_definitions(-DLIB_NDOF=1)
endif (WINDOWS OR DARWIN OR LINUX)

if (WINDOWS)
  set(NDOF_LIBRARY libndofdev)
elseif (DARWIN OR LINUX)
  set(NDOF_LIBRARY ndofdev)
endif (WINDOWS)
