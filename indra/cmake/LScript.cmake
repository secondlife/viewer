# -*- cmake -*-

set(LSCRIPT_INCLUDE_DIRS
    ${LIBS_OPEN_DIR}/lscript
    ${LIBS_OPEN_DIR}/lscript/lscript_compile
    ${LIBS_OPEN_DIR}/lscript/lscript_execute
    )

set(LSCRIPT_LIBRARIES
    lscript_compile
    lscript_execute
    lscript_library
    )
