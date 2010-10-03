# -*- cmake -*-

# - Find FMOD
# Find the FMOD includes and library
# This module defines
#  FMOD_INCLUDE_DIR, where to find fmod.h and fmod_errors.h
#  FMOD_LIBRARIES, the libraries needed to use FMOD.
#  FMOD, If false, do not try to use FMOD.
# also defined, but not for general use are
#  FMOD_LIBRARY, where to find the FMOD library.

FIND_PATH(FMOD_INCLUDE_DIR fmod.h
/usr/local/include/fmod
/usr/local/include
/usr/include/fmod
/usr/include
)

SET(FMOD_NAMES ${FMOD_NAMES} fmod fmodvc fmod-3.75)
FIND_LIBRARY(FMOD_LIBRARY
  NAMES ${FMOD_NAMES}
  PATHS /usr/lib /usr/local/lib
  )

IF (FMOD_LIBRARY AND FMOD_INCLUDE_DIR)
    SET(FMOD_LIBRARIES ${FMOD_LIBRARY})
    SET(FMOD "YES")
ELSE (FMOD_LIBRARY AND FMOD_INCLUDE_DIR)
  SET(FMOD "NO")
ENDIF (FMOD_LIBRARY AND FMOD_INCLUDE_DIR)


IF (FMOD)
   IF (NOT FMOD_FIND_QUIETLY)
      MESSAGE(STATUS "Found FMOD: ${FMOD_LIBRARIES}")
   ENDIF (NOT FMOD_FIND_QUIETLY)
ELSE (FMOD)
   IF (FMOD_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could not find FMOD library")
   ENDIF (FMOD_FIND_REQUIRED)
ENDIF (FMOD)

# Deprecated declarations.
SET (NATIVE_FMOD_INCLUDE_PATH ${FMOD_INCLUDE_DIR} )
GET_FILENAME_COMPONENT (NATIVE_FMOD_LIB_PATH ${FMOD_LIBRARY} PATH)

MARK_AS_ADVANCED(
  FMOD_LIBRARY
  FMOD_INCLUDE_DIR
  )
