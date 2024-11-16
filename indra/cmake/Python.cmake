# -*- cmake -*-

set(PYTHONINTERP_FOUND)

if (DEFINED ENV{PYTHON})
  # Allow python executable to be explicitly set
  set(python "$ENV{PYTHON}")
  set(PYTHONINTERP_FOUND ON)
elseif (WINDOWS)
  # On Windows, explicitly avoid Cygwin Python.

  # if the user has their own version of Python installed, prefer that
  foreach(hive HKEY_CURRENT_USER HKEY_LOCAL_MACHINE)
    # prefer more recent Python versions to older ones, if multiple versions
    # are installed
    foreach(pyver 3.14 3.13 3.12 3.11 3.10 3.9 3.8 3.7)
      list(APPEND regpaths "[${hive}\\SOFTWARE\\Python\\PythonCore\\${pyver}\\InstallPath]")
    endforeach()
  endforeach()

  # TODO: This logic has the disadvantage that if you have multiple versions
  # of Python installed, the selected path won't necessarily be the newest -
  # e.g. this GLOB will prefer Python310 to Python311. But since pymaybe is
  # checked AFTER the registry entries, this will only surface as a problem if
  # no installed Python appears in the registry.
  file(GLOB pymaybe
    "$ENV{PROGRAMFILES}/Python*"
##  "$ENV{PROGRAMFILES(X86)}/Python*"
    # The Windows environment variable is in fact as shown above, but CMake
    # disallows querying an environment variable containing parentheses -
    # thanks, Windows. Fudge by just appending " (x86)" to $PROGRAMFILES and
    # hoping for the best.
    "$ENV{PROGRAMFILES} (x86)/Python*"
    "c:/Python*")

  find_program(python
    NAMES python3.exe python.exe
    NO_DEFAULT_PATH # added so that cmake does not find cygwin python
    PATHS
    ${regpaths}
    ${pymaybe}
    )
  find_package(Python3 COMPONENTS Interpreter)
else()
  find_program(python python3)

  if (python)
    set(PYTHONINTERP_FOUND ON)
  endif (python)
endif (DEFINED ENV{PYTHON})

if (NOT python)
  message(FATAL_ERROR "No Python interpreter found")
endif (NOT python)

set(PYTHON_EXECUTABLE "${python}" CACHE FILEPATH "Python interpreter for builds")
mark_as_advanced(PYTHON_EXECUTABLE)
