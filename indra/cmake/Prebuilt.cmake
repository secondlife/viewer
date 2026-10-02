# -*- cmake -*-
include_guard()

include(FindAutobuild)

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
    if( NOT DEFINED ${_binary}_installed )
        set( ${_binary}_installed "")
    endif()

    if("${${_binary}_installed}" STREQUAL "" AND EXISTS "${PREBUILD_TRACKING_DIR}/${_binary}_installed")
        file(READ ${PREBUILD_TRACKING_DIR}/${_binary}_installed "${_binary}_installed")
        if(DEBUG_PREBUILT)
            message(STATUS "${_binary}_installed: \"${${_binary}_installed}\"")
        endif(DEBUG_PREBUILT)
    endif("${${_binary}_installed}" STREQUAL "" AND EXISTS "${PREBUILD_TRACKING_DIR}/${_binary}_installed")

    if(${PREBUILD_TRACKING_DIR}/sentinel_installed IS_NEWER_THAN ${PREBUILD_TRACKING_DIR}/${_binary}_installed OR NOT ${${_binary}_installed} EQUAL 0)
        if(DEBUG_PREBUILT)
            message(STATUS "cd ${CMAKE_SOURCE_DIR} && ${AUTOBUILD_EXECUTABLE} install
        --install-dir=${AUTOBUILD_INSTALL_DIR}
        ${_binary} ")
        endif(DEBUG_PREBUILT)
        message(STATUS "Installing ${_binary}...")

        # Retry transient download failures (e.g. remote disconnects) instead
        # of failing the whole configure on the first attempt.
        set(_max_retries 4)
        set(_retry_delay 8)
        set(${_binary}_installed 1)

        foreach(_attempt RANGE 1 ${_max_retries})
            if(DEBUG_PREBUILT)
                message(STATUS "autobuild install ${_binary}: attempt ${_attempt}/${_max_retries}")
            endif(DEBUG_PREBUILT)
            execute_process(COMMAND "${AUTOBUILD_EXECUTABLE}"
                    install
                    --skip-source-environment
                    --install-dir=${AUTOBUILD_INSTALL_DIR}
                    ${_binary}
                    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
                    RESULT_VARIABLE ${_binary}_installed
                    )

            if(${${_binary}_installed} EQUAL 0)
                break()
            endif()

            if(_attempt LESS _max_retries)
                message(WARNING "autobuild install ${_binary} failed (rc=${${_binary}_installed}); retrying in ${_retry_delay}s")
                execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep ${_retry_delay})
            endif()
        endforeach()

        file(WRITE ${PREBUILD_TRACKING_DIR}/${_binary}_installed "${${_binary}_installed}")
    endif(${PREBUILD_TRACKING_DIR}/sentinel_installed IS_NEWER_THAN ${PREBUILD_TRACKING_DIR}/${_binary}_installed OR NOT ${${_binary}_installed} EQUAL 0)

    if(NOT ${_binary}_installed EQUAL 0)
        message(FATAL_ERROR
                "Failed to download or unpack prebuilt '${_binary}'."
                " Process returned ${${_binary}_installed}.")
    endif (NOT ${_binary}_installed EQUAL 0)
endmacro (use_prebuilt_binary _binary)

#Sadly we need a macro here, otherwise the return() will not properly work
macro ( use_system_binary package )
  if( USE_CONAN )
    target_link_libraries( ll::${package} INTERFACE CONAN_PKG::${package} )
    foreach( extra_pkg "${ARGN}" )
      if( extra_pkg )
        target_link_libraries( ll::${package} INTERFACE CONAN_PKG::${extra_pkg} )
      endif()
    endforeach()
    return()
  endif()
endmacro()

