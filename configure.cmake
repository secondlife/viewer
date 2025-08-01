#!/usr/bin/env cmake -P

# cmake 3.20 introduces cmake_path()
cmake_minimum_required(VERSION "3.20" FATAL_ERROR)

string(TOLOWER ${CMAKE_HOST_SYSTEM_NAME} SYSTEM)
set(ARCH "x86_64")
set(BUILD_DIR "build-${SYSTEM}-${ARCH}")

# note - must use CMAKE_CURRENT_LIST_DIR when running in `cmake -P` mode
# CMAKE_CURRENT_SOURCE_DIR is misleadingly set to the current working directory
# https://cmake.org/cmake/help/latest/variable/CMAKE_CURRENT_SOURCE_DIR.html
set(TOP_DIR "${CMAKE_CURRENT_LIST_DIR}")
cmake_path(APPEND FULL_BUILD_DIR "${TOP_DIR}" "${BUILD_DIR}")
cmake_path(APPEND FULL_SOURCE_DIR "${TOP_DIR}" "indra")
message("Configuring build directory in ${FULL_BUILD_DIR} for ${FULL_SOURCE_DIR}")

if(NOT DEFINED ENV{AUTOBUILD_VARIABLES_FILE})
  cmake_path(APPEND VARIABLES_FILE_PATH "${FULL_SOURCE_DIR}" "newview" "default_variables")
  set(ENV{AUTOBUILD_VARIABLES_FILE} "${VARIABLES_FILE_PATH}")
  message(INFO " AUTOBUILD_VARIABLES_FILE not in environment variables, setting to default $ENV{AUTOBUILD_VARIABLES_FILE}")
endif()

if(NOT DEFINED ENV{AUTOBUILD_ADDRSIZE})
    set(ENV{AUTOBUILD_ADDRSIZE} "64")
endif()

if(NOT DEFINED GENERATOR)
   if(${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Windows")
     set(GENERATOR "Visual Studio")
   elseif(${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Darwin")
     set(GENERATOR "Xcode")
  else()
    set(GENERATOR "Ninja Multi-Config")
  endif()
endif()

string(TOUPPER "LL_${CMAKE_HOST_SYSTEM_NAME}" LL_SYSTEM_DEF)
set(LL_BUILD "-std=c++20 -fPIC -D${LL_SYSTEM_DEF}=1")

# Find Python executable
find_program(PYTHON_EXECUTABLE python3 python)
if(NOT PYTHON_EXECUTABLE)
  message(FATAL_ERROR "Python not found. Please install Python 3.10 or later.")
endif()

message(INFO " Found Python: ${PYTHON_EXECUTABLE}")

# Create Python virtual environment in build directory
cmake_path(APPEND VENV_PATH "${FULL_BUILD_DIR}" "venv")
if(NOT EXISTS "${VENV_PATH}")
    message(INFO " Creating Python virtual environment at ${VENV_PATH}")
    execute_process(
      COMMAND ${PYTHON_EXECUTABLE} -m venv "${VENV_PATH}"
      RESULT_VARIABLE VENV_RESULT
    )
    if(NOT VENV_RESULT EQUAL 0)
      message(FATAL_ERROR "Failed to create Python virtual environment")
    endif()
    message(INFO " Python virtual environment created successfully at ${VENV_PATH}")
else()
    message(INFO " Python virtual environment already exists at ${VENV_PATH}")
endif()

if(${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Windows")
  cmake_path(APPEND VENV_PYTHON "${VENV_PATH}" "Scripts" "python")
  cmake_path(APPEND VENV_PIP "${VENV_PATH}" "Scripts" "pip")
  cmake_path(APPEND VENV_AUTOBUILD "${VENV_PATH}" "Scripts" "autobuild")
else()
  cmake_path(APPEND VENV_PYTHON "${VENV_PATH}" "bin" "python")
  cmake_path(APPEND VENV_PIP "${VENV_PATH}" "bin" "pip")
  cmake_path(APPEND VENV_AUTOBUILD "${VENV_PATH}" "bin" "autobuild")
endif()

set(ENV{PYTHON} "${VENV_PYTHON}")

message(INFO " installing autobuild and llsd in python venv")
execute_process(
  COMMAND "${VENV_PIP}" install llsd autobuild
  RESULT_VARIABLE PIP_RESULT
)
if(NOT PIP_RESULT EQUAL 0)
  message(FATAL_ERROR "Failed to install autobuild and llsd Python virtual environment")
endif()

message(INFO " executing cmake -G ${GENERATOR} -B ${FULL_BUILD_DIR} -S ${FULL_SOURCE_DIR} -DLL_BUILD_ENV='${LL_BUILD}' -DAUTOBUILD_EXECUTABLE='${VENV_AUTOBUILD}'")
execute_process(
  COMMAND cmake -G "${GENERATOR}" -B "${FULL_BUILD_DIR}" -S "${FULL_SOURCE_DIR}" -DLL_BUILD_ENV='${LL_BUILD}' -DAUTOBUILD_EXECUTABLE='${VENV_AUTOBUILD}'
  RESULT_VARIABLE CMAKE_RESULT
)
if(NOT CMAKE_RESULT EQUAL 0)
  message(FATAL_ERROR "Failed to configure project using CMake")
endif()


message("")
message("To build the viewer, now run the command `cmake --build \"${FULL_BUILD_DIR}\"`")
message("")
