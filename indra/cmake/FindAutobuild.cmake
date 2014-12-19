# -*- cmake -*-
#
# Find the autobuild tool
#
# Output variables:
#
#   AUTOBUILD_EXECUTABLE - path to autobuild or pautobuild executable



IF (NOT AUTOBUILD_EXECUTABLE)

  # If cmake was executed by autobuild, autobuild will have set the AUTOBUILD env var
  IF (DEFINED ENV{AUTOBUILD})
    SET(AUTOBUILD_EXECUTABLE $ENV{AUTOBUILD})
    # In case $AUTOBUILD is a cygwin path, fix it back to Windows style
    STRING(REGEX REPLACE "^/cygdrive/(.)/" "\\1:/" AUTOBUILD_EXECUTABLE
           "${AUTOBUILD_EXECUTABLE}")
  ELSE (DEFINED ENV{AUTOBUILD})
    IF(WIN32)
      SET(AUTOBUILD_EXE_NAMES autobuild.cmd autobuild.exe)
    ELSE(WIN32)
      SET(AUTOBUILD_EXE_NAMES autobuild)
    ENDIF(WIN32)

    SET(AUTOBUILD_EXECUTABLE)
    FIND_PROGRAM(
      AUTOBUILD_EXECUTABLE 
      NAMES ${AUTOBUILD_EXE_NAMES}
      PATHS 
    ENV PATH
    ${CMAKE_SOURCE_DIR}/.. 
    ${CMAKE_SOURCE_DIR}/../..
    ${CMAKE_SOURCE_DIR}/../../..
      PATH_SUFFIXES "/autobuild/bin/"
    )
  ENDIF (DEFINED ENV{AUTOBUILD})

  IF (AUTOBUILD_EXECUTABLE)
    GET_FILENAME_COMPONENT(_autobuild_name ${AUTOBUILD_EXECUTABLE} NAME_WE)
  ELSE (AUTOBUILD_EXECUTABLE)
    IF (AUTOBUILD_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could not find autobuild executable")
    ENDIF (AUTOBUILD_FIND_REQUIRED)
  ENDIF (AUTOBUILD_EXECUTABLE)

  MARK_AS_ADVANCED(AUTOBUILD_EXECUTABLE)
ENDIF (NOT AUTOBUILD_EXECUTABLE)
