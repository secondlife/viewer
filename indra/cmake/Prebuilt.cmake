# -*- cmake -*-

if(NOT DEFINED ${CMAKE_CURRENT_LIST_FILE}_INCLUDED)
set(${CMAKE_CURRENT_LIST_FILE}_INCLUDED "YES")

include(FindAutobuild)
if(INSTALL_PROPRIETARY)
  include(FindSCP)
endif(INSTALL_PROPRIETARY)

set(PREBUILD_TRACKING_DIR ${AUTOBUILD_INSTALL_DIR}/cmake_tracking)
# For the library installation process;
# see cmake/Prebuild.cmake for the counterpart code.
if ("${CMAKE_SOURCE_DIR}/../autobuild.xml" IS_NEWER_THAN "${PREBUILD_TRACKING_DIR}/sentinel_installed")
  file(MAKE_DIRECTORY ${PREBUILD_TRACKING_DIR})
  file(WRITE ${PREBUILD_TRACKING_DIR}/sentinel_installed "0")
endif ("${CMAKE_SOURCE_DIR}/../autobuild.xml" IS_NEWER_THAN "${PREBUILD_TRACKING_DIR}/sentinel_installed")

# The use_prebuilt_binary macro handles automated installation of package
# dependencies using autobuild.  The goal is that 'autobuild install' should
# only be run when we know we need to install a new package.  This should be
# the case in a clean checkout, or if autobuild.xml has been updated since the
# last run (encapsulated by the file ${PREBUILD_TRACKING_DIR}/sentinel_installed),
# or if a previous attempt to install the package has failed (the exit status
# of previous attempts is serialized in the file
# ${PREBUILD_TRACKING_DIR}/${_binary}_installed)
macro (use_prebuilt_binary _binary)
  if (NOT DEFINED USESYSTEMLIBS_${_binary})
    set(USESYSTEMLIBS_${_binary} ${USESYSTEMLIBS})
  endif (NOT DEFINED USESYSTEMLIBS_${_binary})

  if (NOT USESYSTEMLIBS_${_binary})
    if("${${_binary}_installed}" STREQUAL "" AND EXISTS "${PREBUILD_TRACKING_DIR}/${_binary}_installed")
      file(READ ${PREBUILD_TRACKING_DIR}/${_binary}_installed "${_binary}_installed")
      if(DEBUG_PREBUILT)
        message(STATUS "${_binary}_installed: \"${${_binary}_installed}\"")
      endif(DEBUG_PREBUILT)
    endif("${${_binary}_installed}" STREQUAL "" AND EXISTS "${PREBUILD_TRACKING_DIR}/${_binary}_installed")

    if(${PREBUILD_TRACKING_DIR}/sentinel_installed IS_NEWER_THAN ${PREBUILD_TRACKING_DIR}/${_binary}_installed OR NOT ${${_binary}_installed} EQUAL 0)
      if(DEBUG_PREBUILT)
        message("cd ${CMAKE_SOURCE_DIR} && ${AUTOBUILD_EXECUTABLE} install
        --install-dir=${AUTOBUILD_INSTALL_DIR}
        ${_binary} ")
      endif(DEBUG_PREBUILT)
      execute_process(COMMAND "${AUTOBUILD_EXECUTABLE}"
        install
        --install-dir=${AUTOBUILD_INSTALL_DIR}
        ${_binary}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        RESULT_VARIABLE ${_binary}_installed
        )
      file(WRITE ${PREBUILD_TRACKING_DIR}/${_binary}_installed "${${_binary}_installed}")
    endif(${PREBUILD_TRACKING_DIR}/sentinel_installed IS_NEWER_THAN ${PREBUILD_TRACKING_DIR}/${_binary}_installed OR NOT ${${_binary}_installed} EQUAL 0)

    if(NOT ${_binary}_installed EQUAL 0)
      message(FATAL_ERROR
              "Failed to download or unpack prebuilt '${_binary}'."
              " Process returned ${${_binary}_installed}.")
    endif (NOT ${_binary}_installed EQUAL 0)
  endif (NOT USESYSTEMLIBS_${_binary})
endmacro (use_prebuilt_binary _binary)

endif(NOT DEFINED ${CMAKE_CURRENT_LIST_FILE}_INCLUDED)
