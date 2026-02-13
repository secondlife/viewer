include_guard()

# Allow explicit Python path via environment variable
if(DEFINED ENV{PYTHON})
    set(Python3_ROOT_DIR "$ENV{PYTHON}")
endif()

# On Windows, prefer registry entries to avoid Cygwin/MSYS Python
# The registry is searched first by default, which finds native Windows Python
# installations rather than Cygwin/MSYS Python
if(WINDOWS)
    set(Python3_FIND_REGISTRY FIRST CACHE STRING "Python search order")
endif()

# Find Python 3 interpreter
find_package(Python3 REQUIRED COMPONENTS Interpreter)
