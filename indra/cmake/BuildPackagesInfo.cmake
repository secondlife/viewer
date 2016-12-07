# -*- cmake -*-
# Construct the version and copyright information based on package data.
include(Python)

# packages-formatter.py runs autobuild install --versions, which needs to know
# the build_directory, which (on Windows) depends on AUTOBUILD_ADDRSIZE.
# Within an autobuild build, AUTOBUILD_ADDRSIZE is already set. But when
# building in an IDE, it probably isn't. Set it explicitly using
# run_build_test.py.
add_custom_command(OUTPUT packages-info.txt
  COMMENT Generating packages-info.txt for the about box
  MAIN_DEPENDENCY ${CMAKE_SOURCE_DIR}/../autobuild.xml
  DEPENDS ${CMAKE_SOURCE_DIR}/../scripts/packages-formatter.py
  COMMAND ${PYTHON_EXECUTABLE}
          ${CMAKE_SOURCE_DIR}/cmake/run_build_test.py -DAUTOBUILD_ADDRSIZE=${ADDRESS_SIZE}
          ${PYTHON_EXECUTABLE}
          ${CMAKE_SOURCE_DIR}/../scripts/packages-formatter.py "${VIEWER_CHANNEL}" "${VIEWER_SHORT_VERSION}.${VIEWER_VERSION_REVISION}" > packages-info.txt
  )
