MACRO(LL_TEST_COMMAND OUTVAR LD_LIBRARY_PATH)
  # nat wonders how Kitware can use the term 'function' for a construct that
  # cannot return a value. And yet, variables you set inside a FUNCTION are
  # local. Try a MACRO instead.
  SET(value
    ${PYTHON_EXECUTABLE}
    "${CMAKE_SOURCE_DIR}/cmake/run_build_test.py")
  IF(LD_LIBRARY_PATH)
    LIST(APPEND value "-l${LD_LIBRARY_PATH}")
  ENDIF(LD_LIBRARY_PATH)
  LIST(APPEND value ${ARGN})
  SET(${OUTVAR} ${value})
##MESSAGE(STATUS "Will run: ${value}")
ENDMACRO(LL_TEST_COMMAND)
