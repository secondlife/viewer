# -*- cmake -*-

# - Find MySQL
# Find the MySQL includes and library
# This module defines
#  MYSQL_INCLUDE_DIR, where to find mysql.h, etc.
#  MYSQL_LIBRARIES, the libraries needed to use Mysql.
#  MYSQL_FOUND, If false, do not try to use Mysql.
# also defined, but not for general use are
#  MYSQL_LIBRARY, where to find the Mysql library.

FIND_PATH(MYSQL_INCLUDE_DIR mysql/mysql.h
/usr/local/include
/usr/include
)

SET(MYSQL_NAMES ${MYSQL_NAMES} mysqlclient)
FIND_LIBRARY(MYSQL_LIBRARY
  NAMES ${MYSQL_NAMES}
  PATHS /usr/lib/mysql /usr/lib /usr/local/lib/mysql /usr/local/lib
  )

IF (MYSQL_LIBRARY AND MYSQL_INCLUDE_DIR)
    SET(MYSQL_LIBRARIES ${MYSQL_LIBRARY})
    SET(MYSQL_FOUND "YES")
ELSE (MYSQL_LIBRARY AND MYSQL_INCLUDE_DIR)
  SET(MYSQL_FOUND "NO")
ENDIF (MYSQL_LIBRARY AND MYSQL_INCLUDE_DIR)


IF (MYSQL_FOUND)
   IF (NOT MYSQL_FIND_QUIETLY)
      MESSAGE(STATUS "Found MySQL: ${MYSQL_LIBRARIES}")
   ENDIF (NOT MYSQL_FIND_QUIETLY)
ELSE (MYSQL_FOUND)
   IF (MYSQL_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could not find MySQL library")
   ENDIF (MYSQL_FIND_REQUIRED)
ENDIF (MYSQL_FOUND)

# Deprecated declarations.
SET (NATIVE_MYSQL_INCLUDE_PATH ${MYSQL_INCLUDE_DIR} )
GET_FILENAME_COMPONENT (NATIVE_MYSQL_LIB_PATH ${MYSQL_LIBRARY} PATH)

MARK_AS_ADVANCED(
  MYSQL_LIBRARY
  MYSQL_INCLUDE_DIR
  )
