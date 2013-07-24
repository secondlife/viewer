# -*- cmake -*-

include(00-Common)
include(LLCommon)
include(LLMath)
include(LLMessage)
include(LLInventory)
include(LLPrimitive)
include(LScript)

include(FindCygwin)

find_program(FLEX flex
             "C:/Program Files/GnuWin32/bin"
             ${CYGWIN_INSTALL_PATH}/bin
             /bin
             /usr/bin
             /usr/local/bin
             )
mark_as_advanced(FLEX)

find_program(BISON bison
             "C:/Program Files/GnuWin32/bin"
             ${CYGWIN_INSTALL_PATH}/bin
             /bin
             /usr/bin
             /usr/local/bin
             )
mark_as_advanced(BISON)

find_program(M4 m4
             "C:/Program Files/GnuWin32/bin"
             ${CYGWIN_INSTALL_PATH}/bin
             /bin
             /usr/bin
             /usr/local/bin
             )
mark_as_advanced(M4)

include_directories(
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
    ${LLMESSAGE_INCLUDE_DIRS}
    ${LLINVENTORY_INCLUDE_DIRS}
    ${LLPRIMITIVE_INCLUDE_DIRS}
    ${LSCRIPT_INCLUDE_DIRS}
    )
include_directories(SYSTEM
    ${LLCOMMON_SYSTEM_INCLUDE_DIRS}
    )

set(lscript_generated_SOURCE_FILES
    indra.l.cpp
    indra.y.cpp
    )

set(lscript_compile_SOURCE_FILES
    lscript_alloc.cpp
    lscript_bytecode.cpp
    lscript_error.cpp
    lscript_heap.cpp
    lscript_resource.cpp
    lscript_scope.cpp
    lscript_tree.cpp
    lscript_typecheck.cpp
    )
    
set(lscript_compile_HEADER_FILES
    CMakeLists.txt

    indra.l
    indra.y

    ../lscript_alloc.h
    ../lscript_byteformat.h
    ../lscript_byteconvert.h
    ../lscript_http.h

    lscript_error.h
    lscript_bytecode.h
    lscript_heap.h
    lscript_resource.h
    lscript_scope.h
    lscript_tree.h
    lscript_typecheck.h
    )

set_source_files_properties(${lscript_compile_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

set_source_files_properties(${lscript_generated_SOURCE_FILES}
                            PROPERTIES HEADER_FILE_ONLY FALSE GENERATED TRUE)

list(APPEND lscript_compile_SOURCE_FILES ${lscript_generated_SOURCE_FILES} ${lscript_compile_HEADER_FILES})

add_custom_command(
    OUTPUT
      ${CMAKE_CURRENT_BINARY_DIR}/indra.l.cpp
    COMMAND ${FLEX}
    ARGS
      -P indra_
      -o${CMAKE_CURRENT_BINARY_DIR}/indra.l.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/indra.l
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/indra.l
    )

if (WINDOWS)
  set_source_files_properties(indra.l.cpp
                              PROPERTIES COMPILE_FLAGS /DYY_NO_UNISTD_H)
endif (WINDOWS)

if (WINDOWS)
  get_filename_component(M4_PATH ${M4} PATH)
  add_custom_command(
      OUTPUT
        ${CMAKE_CURRENT_BINARY_DIR}/indra.y.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/indra.y.hpp
      COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/bison.bat
      ARGS
        ${BISON} ${M4_PATH}
        -p indra_
        -d -o ${CMAKE_CURRENT_BINARY_DIR}/indra.y.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/indra.y
      DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/bison.bat
        ${CMAKE_CURRENT_SOURCE_DIR}/indra.y
      )
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/windows)
else (WINDOWS)
  add_custom_command(
      OUTPUT
        ${CMAKE_CURRENT_BINARY_DIR}/indra.y.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/indra.y.hpp
      COMMAND
        ${BISON}
      ARGS
        -p indra_
        -d -o ${CMAKE_CURRENT_BINARY_DIR}/indra.y.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/indra.y
      DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/indra.y
      )
endif (WINDOWS)

if (DARWIN)
  # Mac OS X 10.4 compatibility
  add_custom_command(
    OUTPUT
      ${CMAKE_CURRENT_BINARY_DIR}/indra.y.hpp
    COMMAND
      mv
      ${CMAKE_CURRENT_BINARY_DIR}/indra.y.cpp.h
      ${CMAKE_CURRENT_BINARY_DIR}/indra.y.hpp
    )
endif (DARWIN)

add_library (lscript_compile ${lscript_compile_SOURCE_FILES})
