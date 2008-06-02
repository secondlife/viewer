# -*- cmake -*-

if (WINDOWS OR DARWIN)
  add_definitions(-DLIB_NDOF=1)
endif (WINDOWS OR DARWIN)

if (WINDOWS)
  set(NDOF_LIBRARY libndofdev)
elseif (DARWIN)
  set(NDOF_LIBRARY ndofdev)
endif (WINDOWS)
