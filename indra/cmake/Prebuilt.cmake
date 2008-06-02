# -*- cmake -*-

include(Python)

macro (use_prebuilt_library _lib)
  if (NOT STANDALONE)
    exec_program(${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}
                 ARGS
                 --install-dir=${LIBS_PREBUILT_DIR} ${_lib}/${ARCH}
                 RETURN_VALUE _installed
                 )
    if (NOT _installed)
      message(FATAL_ERROR
              "Failed to download or unpack prebuilt ${_lib} for ${ARCH}")
    endif (NOT _installed)
  endif (NOT STANDALONE)
endmacro (use_prebuilt_library _lib)
