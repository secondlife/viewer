# -*- cmake -*-
#
# Find the autobuild tool
#
# Output variables:
#
#   AUTOBUILD_EXECUTABLE - path to autobuild or pautobuild executable

# *TODO - if cmake was executed by autobuild, autobuild will have set the AUTOBUILD env var
# update this to check for that case

IF (NOT AUTOBUILD_EXECUTABLE)
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

    IF (AUTOBUILD_EXECUTABLE)
      GET_FILENAME_COMPONENT(_autobuild_name ${AUTOBUILD_EXECUTABLE} NAME_WE)
      MESSAGE(STATUS "Using autobuild at: ${AUTOBUILD_EXECUTABLE}")
    ELSE (AUTOBUILD_EXECUTABLE)
      IF (AUTOBUILD_FIND_REQUIRED)
	MESSAGE(FATAL_ERROR "Could not find autobuild executable")
      ENDIF (AUTOBUILD_FIND_REQUIRED)
    ENDIF (AUTOBUILD_EXECUTABLE)

    MARK_AS_ADVANCED(AUTOBUILD_EXECUTABLE)
ENDIF (NOT AUTOBUILD_EXECUTABLE)
