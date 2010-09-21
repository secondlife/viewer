# -*- cmake -*-
#
# Find the svn executable for exporting old svn:externals.
#
# Input variables:
#   SVN_FIND_REQUIRED - set this if configuration should fail without scp
#
# Output variables:
#
#   SVN_FOUND - set if svn was found
#   SVN_EXECUTABLE - path to svn executable
#   SVN_BATCH_FLAG - how to put svn into batch mode


SET(SVN_EXECUTABLE)
FIND_PROGRAM(SVN_EXECUTABLE NAMES svn svn.exe)

IF (SVN_EXECUTABLE)
  SET(SVN_FOUND ON)
ELSE (SVN_EXECUTABLE)
  SET(SVN_FOUND OFF)
ENDIF (SVN_EXECUTABLE)

IF (SVN_FOUND)
  GET_FILENAME_COMPONENT(_svn_name ${SVN_EXECUTABLE} NAME_WE)
  SET(SVN_BATCH_FLAG --non-interactive)
ELSE (SVN_FOUND)
  IF (SVN_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Could not find svn executable")
  ENDIF (SVN_FIND_REQUIRED)
ENDIF (SVN_FOUND)

MARK_AS_ADVANCED(SVN_EXECUTABLE SVN_FOUND SVN_BATCH_FLAG)

