# -*- cmake -*-
# Construct the version and copyright information based on package data.
include(Python)

add_custom_command(OUTPUT packages-info.txt
  COMMENT Generating packages-info.txt for the about box
  MAIN_DEPENDENCY ${CMAKE_SOURCE_DIR}/../autobuild.xml
  DEPENDS ${CMAKE_SOURCE_DIR}/../scripts/packages-formatter.py
  COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/../scripts/packages-formatter.py > packages-info.txt
  )
