# -*- cmake -*-

set(PYTHONINTERP_FOUND)

if (WINDOWS)
  # On Windows, explicitly avoid Cygwin Python.

  find_program(PYTHON_EXECUTABLE
    NAMES python25.exe python23.exe python.exe
    NO_DEFAULT_PATH # added so that cmake does not find cygwin python
    PATHS
    [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.7\\InstallPath]
    [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.6\\InstallPath]
    [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.5\\InstallPath]
    [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.4\\InstallPath]
    [HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore\\2.3\\InstallPath]
    [HKEY_CURRENT_USER\\SOFTWARE\\Python\\PythonCore\\2.7\\InstallPath]
    [HKEY_CURRENT_USER\\SOFTWARE\\Python\\PythonCore\\2.6\\InstallPath]
    [HKEY_CURRENT_USER\\SOFTWARE\\Python\\PythonCore\\2.5\\InstallPath]
    [HKEY_CURRENT_USER\\SOFTWARE\\Python\\PythonCore\\2.4\\InstallPath]
    [HKEY_CURRENT_USER\\SOFTWARE\\Python\\PythonCore\\2.3\\InstallPath]
    )
elseif (EXISTS /etc/debian_version)
  # On Debian and Ubuntu, avoid Python 2.4 if possible.

  find_program(PYTHON_EXECUTABLE python PATHS /usr/bin)

  if (PYTHON_EXECUTABLE)
    set(PYTHONINTERP_FOUND ON)
  endif (PYTHON_EXECUTABLE)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  # On MAC OS X be sure to search standard locations first

  string(REPLACE ":" ";" PATH_LIST "$ENV{PATH}")
  find_program(PYTHON_EXECUTABLE
    NAMES python python25 python24 python23
    NO_DEFAULT_PATH # Avoid searching non-standard locations first
    PATHS
    /bin
    /usr/bin
    /usr/local/bin
    ${PATH_LIST}
    )

  if (PYTHON_EXECUTABLE)
    set(PYTHONINTERP_FOUND ON)
  endif (PYTHON_EXECUTABLE)
else (WINDOWS)
  include(FindPythonInterp)
endif (WINDOWS)

if (NOT PYTHON_EXECUTABLE)
  message(FATAL_ERROR "No Python interpreter found")
endif (NOT PYTHON_EXECUTABLE)

mark_as_advanced(PYTHON_EXECUTABLE)
