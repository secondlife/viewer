# -*- cmake -*-

include(Python)
include(FindSVN)

macro (use_svn_external _binary _path _url _rev)
  if (NOT STANDALONE)
    if(${CMAKE_BINARY_DIR}/temp/sentinel_installed IS_NEWER_THAN ${CMAKE_BINARY_DIR}/temp/${_binary}_installed)
      if(SVN_FOUND)
        if(DEBUG_EXTERNALS)
          message("cd ${_path} && ${SVN_EXECUTABLE} checkout -r ${_rev} ${_url} ${_binary}")
        endif(DEBUG_EXTERNALS)
        execute_process(COMMAND ${SVN_EXECUTABLE}
          checkout
          -r ${_rev}
          ${_url}
          ${_binary}
          WORKING_DIRECTORY ${_path}
          RESULT_VARIABLE ${_binary}_installed
          )
      else(SVN_FOUND)
        message(FATAL_ERROR "Failed to find SVN_EXECUTABLE")
      endif(SVN_FOUND)
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
endmacro (use_svn_external _binary _path _url _rev)
