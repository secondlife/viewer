# -*- cmake -*-

if(NOT DEFINED ${CMAKE_CURRENT_LIST_FILE}_INCLUDED)
set(${CMAKE_CURRENT_LIST_FILE}_INCLUDED "YES")

include(FindAutobuild)
if(INSTALL_PROPRIETARY)
  include(FindSCP)
endif(INSTALL_PROPRIETARY)

# The use_prebuilt_binary macro handles automated installation of package
# dependencies using autobuild.  The goal is that 'autobuild install' should
# only be run when we know we need to install a new package.  This should be
# the case in a clean checkout, or if autobuild.xml has been updated since the
# last run (encapsulated by the file ${CMAKE_BINARY_DIR}/temp/sentinel_installed),
# or if a previous attempt to install the package has failed (the exit status
# of previous attempts is serialized in the file
# ${CMAKE_BINARY_DIR}/temp/${_binary}_installed)
macro (use_prebuilt_binary _binary)
  if (NOT DEFINED STANDALONE_${_binary})
    set(STANDALONE_${_binary} ${STANDALONE})
  endif (NOT DEFINED STANDALONE_${_binary})

  if (NOT STANDALONE_${_binary})
    if("${${_binary}_installed}" STREQUAL "" AND EXISTS "${CMAKE_BINARY_DIR}/temp/${_binary}_installed")
      file(READ ${CMAKE_BINARY_DIR}/temp/${_binary}_installed "${_binary}_installed")
      if(DEBUG_PREBUILT)
        message(STATUS "${_binary}_installed: \"${${_binary}_installed}\"")
      endif(DEBUG_PREBUILT)
    endif("${${_binary}_installed}" STREQUAL "" AND EXISTS "${CMAKE_BINARY_DIR}/temp/${_binary}_installed")

    if(${CMAKE_BINARY_DIR}/temp/sentinel_installed IS_NEWER_THAN ${CMAKE_BINARY_DIR}/temp/${_binary}_installed OR NOT ${${_binary}_installed} EQUAL 0)
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
    endif(${CMAKE_BINARY_DIR}/temp/sentinel_installed IS_NEWER_THAN ${CMAKE_BINARY_DIR}/temp/${_binary}_installed OR NOT ${${_binary}_installed} EQUAL 0)

    if(NOT ${_binary}_installed EQUAL 0)
      message(FATAL_ERROR
              "Failed to download or unpack prebuilt '${_binary}'."
              " Process returned ${${_binary}_installed}.")
    endif (NOT ${_binary}_installed EQUAL 0)
  endif (NOT STANDALONE_${_binary})
endmacro (use_prebuilt_binary _binary)

endif(NOT DEFINED ${CMAKE_CURRENT_LIST_FILE}_INCLUDED)
