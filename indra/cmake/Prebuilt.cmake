# -*- cmake -*-

include(FindAutobuild)

macro (use_prebuilt_binary _binary)
  if (NOT DEFINED STANDALONE_${_binary})
    set(STANDALONE_${_binary} ${STANDALONE})
  endif (NOT DEFINED STANDALONE_${_binary})

  if (NOT STANDALONE_${_binary})
    if(${CMAKE_BINARY_DIR}/temp/sentinel_installed IS_NEWER_THAN ${CMAKE_BINARY_DIR}/temp/${_binary}_installed)
      if(INSTALL_PROPRIETARY)
        include(FindSCP)
      endif(INSTALL_PROPRIETARY)
	  if(DEBUG_PREBUILT)
		message("cd ${CMAKE_SOURCE_DIR} && ${AUTOBUILD_EXECUTABLE} install
		--install-dir=${AUTOBUILD_INSTALL_DIR}
		--skip-license-check
		${_binary} ")
	  endif(DEBUG_PREBUILT)
	  execute_process(COMMAND "${AUTOBUILD_EXECUTABLE}"
		install
		--install-dir=${AUTOBUILD_INSTALL_DIR}
		--skip-license-check
		${_binary}
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
		RESULT_VARIABLE ${_binary}_installed
		)
      file(WRITE ${CMAKE_BINARY_DIR}/temp/${_binary}_installed "${${_binary}_installed}")
    else(${CMAKE_BINARY_DIR}/temp/sentinel_installed IS_NEWER_THAN ${CMAKE_BINARY_DIR}/temp/${_binary}_installed)
      set(${_binary}_installed 0)
    endif(${CMAKE_BINARY_DIR}/temp/sentinel_installed IS_NEWER_THAN ${CMAKE_BINARY_DIR}/temp/${_binary}_installed)
    if(NOT ${_binary}_installed EQUAL 0)
      message(FATAL_ERROR
              "Failed to download or unpack prebuilt '${_binary}'."
              " Process returned ${${_binary}_installed}.")
    endif (NOT ${_binary}_installed EQUAL 0)
  endif (NOT STANDALONE_${_binary})
endmacro (use_prebuilt_binary _binary)
