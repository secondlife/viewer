# -*- cmake -*-

set(PYTHONINTERP_FOUND)

if (DEFINED ENV{PYTHON})
  # Allow python executable to be explicitly set
  set(python "$ENV{PYTHON}")
  set(PYTHONINTERP_FOUND ON)
elseif (WINDOWS)
  if (DEFINED ENV{VIRTUAL_ENV})
    set(Python3_FIND_VIRTUALENV "ONLY")
  endif()
  find_package(Python3 COMPONENTS Interpreter)
  set(python ${Python3_EXECUTABLE})
else()
  find_program(python python3)
endif (DEFINED ENV{PYTHON})

if (python)
  set(PYTHONINTERP_FOUND ON)
else()
  message(FATAL_ERROR "No Python interpreter found")
endif (python)

set(PYTHON_EXECUTABLE "${python}" CACHE FILEPATH "Python interpreter for builds")
mark_as_advanced(PYTHON_EXECUTABLE)
