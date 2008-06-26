# -*- cmake -*-

include(Python)
include(FindSCP)

macro (use_prebuilt_binary _binary)
  if (NOT STANDALONE)
    if(${CMAKE_BINARY_DIR}/temp/sentinel_installed IS_NEWER_THAN ${CMAKE_BINARY_DIR}/temp/${_binary}_installed)
      if(INSTALL_PROPRIETARY)
        include(FindSCP)
        execute_process(COMMAND ${PYTHON_EXECUTABLE}
          install.py 
          --install-dir=${CMAKE_SOURCE_DIR}/..
          --scp=${SCP_EXECUTABLE}
          ${_binary}
          WORKING_DIRECTORY ${SCRIPTS_DIR}
          RESULT_VARIABLE ${_binary}_installed
          )
      else(INSTALL_PROPRIETARY)
        execute_process(COMMAND ${PYTHON_EXECUTABLE}
          install.py 
          --install-dir=${CMAKE_SOURCE_DIR}/..
          ${_binary}
          WORKING_DIRECTORY ${SCRIPTS_DIR}
          RESULT_VARIABLE ${_binary}_installed
          )
      endif(INSTALL_PROPRIETARY)
      file(WRITE ${CMAKE_BINARY_DIR}/temp/${_binary}_installed "${${_binary}_installed}")
    else(${CMAKE_BINARY_DIR}/temp/sentinel_installed IS_NEWER_THAN ${CMAKE_BINARY_DIR}/temp/${_binary}_installed)
      set(${_binary}_installed 0)
    endif(${CMAKE_BINARY_DIR}/temp/sentinel_installed IS_NEWER_THAN ${CMAKE_BINARY_DIR}/temp/${_binary}_installed)
    if(NOT ${_binary}_installed EQUAL 0)
      message(FATAL_ERROR
              "Failed to download or unpack prebuilt '${_binary}'."
              " Process returned ${${_binary}_installed}.")
    endif (NOT ${_binary}_installed EQUAL 0)
  endif (NOT STANDALONE)
endmacro (use_prebuilt_binary _binary)
