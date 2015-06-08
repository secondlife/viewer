# -*- cmake -*-
#
# Find the autobuild tool
#
# Output variables:
#
#   AUTOBUILD_EXECUTABLE - path to autobuild executable



IF (NOT AUTOBUILD_EXECUTABLE)

  # If cmake was executed by autobuild, autobuild will have set the AUTOBUILD env var
  IF (DEFINED ENV{AUTOBUILD})
    SET(AUTOBUILD_EXECUTABLE $ENV{AUTOBUILD})
  ELSE (DEFINED ENV{AUTOBUILD})
    IF(WIN32)
      SET(AUTOBUILD_EXE_NAMES autobuild.exe autobuild.cmd)
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

  IF (NOT AUTOBUILD_EXECUTABLE)
    IF (AUTOBUILD_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could not find autobuild executable")
    ENDIF (AUTOBUILD_FIND_REQUIRED)
  ENDIF (NOT AUTOBUILD_EXECUTABLE)

  MARK_AS_ADVANCED(AUTOBUILD_EXECUTABLE)
ENDIF (NOT AUTOBUILD_EXECUTABLE)
