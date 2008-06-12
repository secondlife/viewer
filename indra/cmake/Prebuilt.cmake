# -*- cmake -*-

include(Python)

macro (use_prebuilt_binary _binary)
  if (NOT STANDALONE)
    execute_process(COMMAND ${PYTHON_EXECUTABLE}
                    install.py 
                    --install-dir=${CMAKE_SOURCE_DIR}/..
                    ${_binary}
                    WORKING_DIRECTORY ${SCRIPTS_DIR}
                    RESULT_VARIABLE _installed
                    )
    if (NOT _installed EQUAL 0)
      message(FATAL_ERROR
              "Failed to download or unpack prebuilt '${_binary}'."
              " Process returned ${_installed}.")
    endif (NOT _installed EQUAL 0)
  endif (NOT STANDALONE)
endmacro (use_prebuilt_binary _binary)
