# -*- cmake -*-
#
# Find the OpenSSH scp ("secure copy") or Putty pscp command.
#
# Input variables:
#   SCP_FIND_REQUIRED - set this if configuration should fail without scp
#
# Output variables:
#
#   SCP_FOUND - set if scp was found
#   SCP_EXECUTABLE - path to scp or pscp executable
#   SCP_BATCH_FLAG - how to put scp/pscp into batch mode

SET(SCP_EXECUTABLE)
IF (WINDOWS)
  FIND_PROGRAM(SCP_EXECUTABLE NAMES pscp pscp.exe)
ELSE (WINDOWS)
  FIND_PROGRAM(SCP_EXECUTABLE NAMES scp scp.exe)
ENDIF (WINDOWS)

IF (SCP_EXECUTABLE)
  SET(SCP_FOUND ON)
ELSE (SCP_EXECUTABLE)
  SET(SCP_FOUND OFF)
ENDIF (SCP_EXECUTABLE)

IF (SCP_FOUND)
  GET_FILENAME_COMPONENT(_scp_name ${SCP_EXECUTABLE} NAME_WE)
  IF (_scp_name STREQUAL scp)
    SET(SCP_BATCH_FLAG -B)
  ELSE (_scp_name STREQUAL scp)
    SET(SCP_BATCH_FLAG -batch)
  ENDIF (_scp_name STREQUAL scp)
ELSE (SCP_FOUND)
  IF (SCP_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Could not find scp or pscp executable")
  ENDIF (SCP_FIND_REQUIRED)
ENDIF (SCP_FOUND)

MARK_AS_ADVANCED(SCP_EXECUTABLE SCP_FOUND SCP_BATCH_FLAG)
