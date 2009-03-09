# -*- cmake -*-

INCLUDE(APR)
INCLUDE(LLMath)

MACRO(ADD_BUILD_TEST_NO_COMMON name parent)
#   MESSAGE("${CMAKE_CURRENT_SOURCE_DIR}/tests/${name}_test.cpp")
    IF (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/${name}_test.cpp")
        SET(no_common_libraries
            ${APRUTIL_LIBRARIES}
            ${APR_LIBRARIES}
            ${PTHREAD_LIBRARY}
            ${WINDOWS_LIBRARIES}
            )
        SET(no_common_source_files
            ${name}.cpp
            tests/${name}_test.cpp
            ${CMAKE_SOURCE_DIR}/test/test.cpp
            )
        ADD_BUILD_TEST_INTERNAL("${name}" "${parent}" "${no_common_libraries}" "${no_common_source_files}")
    ENDIF (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/${name}_test.cpp")
ENDMACRO(ADD_BUILD_TEST_NO_COMMON name parent)


MACRO(ADD_BUILD_TEST name parent)
    # optional extra parameter: list of additional source files
    SET(more_source_files "${ARGN}")

#   MESSAGE("${CMAKE_CURRENT_SOURCE_DIR}/tests/${name}_test.cpp")
    IF (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/${name}_test.cpp")

        SET(basic_libraries
            ${LLCOMMON_LIBRARIES}
            ${APRUTIL_LIBRARIES}
            ${APR_LIBRARIES}
            ${PTHREAD_LIBRARY}
            ${WINDOWS_LIBRARIES}
            )
        SET(basic_source_files
            ${name}.cpp
            tests/${name}_test.cpp
            ${CMAKE_SOURCE_DIR}/test/test.cpp
            ${CMAKE_SOURCE_DIR}/test/lltut.cpp
            ${more_source_files}
            )
        ADD_BUILD_TEST_INTERNAL("${name}" "${parent}" "${basic_libraries}" "${basic_source_files}")

    ENDIF (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/${name}_test.cpp")
ENDMACRO(ADD_BUILD_TEST name parent)


MACRO(ADD_VIEWER_BUILD_TEST name parent)
    # This is just like the generic ADD_BUILD_TEST, but we implicitly
    # add the necessary precompiled header .cpp file (anyone else find that
    # oxymoronic?) because the MSVC build errors will NOT point you there.
    ADD_BUILD_TEST("${name}" "${parent}" llviewerprecompiledheaders.cpp)
ENDMACRO(ADD_VIEWER_BUILD_TEST name parent)


MACRO(ADD_SIMULATOR_BUILD_TEST name parent)
    ADD_BUILD_TEST("${name}" "${parent}" llsimprecompiledheaders.cpp)

    if (WINDOWS)
        SET_SOURCE_FILES_PROPERTIES(
            "tests/${name}_test.cpp"
            PROPERTIES
            COMPILE_FLAGS "/Yullsimprecompiledheaders.h"
        )
    endif (WINDOWS)
ENDMACRO(ADD_SIMULATOR_BUILD_TEST name parent)

MACRO(ADD_BUILD_TEST_INTERNAL name parent libraries source_files)
    # Optional additional parameter: pathname of Python wrapper script
    SET(wrapper "${ARGN}")
    #MESSAGE(STATUS "ADD_BUILD_TEST_INTERNAL ${name} wrapper = ${wrapper}")

    SET(TEST_SOURCE_FILES ${source_files})
    SET(HEADER "${name}.h")
    set_source_files_properties(${HEADER}
                            PROPERTIES HEADER_FILE_ONLY TRUE)
    LIST(APPEND TEST_SOURCE_FILES ${HEADER})
    INCLUDE_DIRECTORIES("${LIBS_OPEN_DIR}/test")
    ADD_EXECUTABLE(${name}_test ${TEST_SOURCE_FILES})
    TARGET_LINK_LIBRARIES(${name}_test
        ${libraries}
        )

    GET_TARGET_PROPERTY(TEST_EXE ${name}_test LOCATION)
    SET(TEST_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${name}_test_ok.txt)

    SET(TEST_CMD ${TEST_EXE} --touch=${TEST_OUTPUT} --sourcedir=${CMAKE_CURRENT_SOURCE_DIR})
    IF (wrapper)
      SET(TEST_CMD ${PYTHON_EXECUTABLE} ${wrapper} ${TEST_CMD})
    ENDIF (wrapper)

    #MESSAGE(STATUS "ADD_BUILD_TEST_INTERNAL ${name} test_cmd  = ${TEST_CMD}")
    SET(TEST_SCRIPT_CMD 
      ${CMAKE_COMMAND} 
      -DLD_LIBRARY_PATH=${ARCH_PREBUILT_DIRS}:/usr/lib
      -DTEST_CMD:STRING="${TEST_CMD}" 
      -P ${CMAKE_SOURCE_DIR}/cmake/RunBuildTest.cmake
      )

    #MESSAGE(STATUS "ADD_BUILD_TEST_INTERNAL ${name} test_script  = ${TEST_SCRIPT_CMD}")
    ADD_CUSTOM_COMMAND(
      OUTPUT ${TEST_OUTPUT}
      COMMAND ${TEST_SCRIPT_CMD}
      DEPENDS ${name}_test
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      )

    ADD_CUSTOM_TARGET(${name}_test_ok ALL DEPENDS ${TEST_OUTPUT})
    IF (${parent})
      ADD_DEPENDENCIES(${parent} ${name}_test_ok)
    ENDIF (${parent})

ENDMACRO(ADD_BUILD_TEST_INTERNAL name parent libraries source_files)


MACRO(ADD_COMM_BUILD_TEST name parent wrapper)
##  MESSAGE(STATUS "ADD_COMM_BUILD_TEST ${name} wrapper = ${wrapper}")
    # optional extra parameter: list of additional source files
    SET(more_source_files "${ARGN}")
##  MESSAGE(STATUS "ADD_COMM_BUILD_TEST ${name} more_source_files = ${more_source_files}")

    SET(libraries
        ${LLMESSAGE_LIBRARIES}
        ${LLMATH_LIBRARIES}
        ${LLVFS_LIBRARIES}
        ${LLCOMMON_LIBRARIES}
        ${APRUTIL_LIBRARIES}
        ${APR_LIBRARIES}
        ${PTHREAD_LIBRARY}
        ${WINDOWS_LIBRARIES}
        )
    SET(source_files
        ${name}.cpp
        tests/${name}_test.cpp
        ${CMAKE_SOURCE_DIR}/test/test.cpp
        ${CMAKE_SOURCE_DIR}/test/lltut.cpp
        ${more_source_files}
        )

    ADD_BUILD_TEST_INTERNAL("${name}" "${parent}" "${libraries}" "${source_files}" "${wrapper}")
ENDMACRO(ADD_COMM_BUILD_TEST name parent wrapper)

MACRO(ADD_VIEWER_COMM_BUILD_TEST name parent wrapper)
    # This is just like the generic ADD_COMM_BUILD_TEST, but we implicitly
    # add the necessary precompiled header .cpp file (anyone else find that
    # oxymoronic?) because the MSVC build errors will NOT point you there.
##  MESSAGE(STATUS "ADD_VIEWER_COMM_BUILD_TEST ${name} wrapper = ${wrapper}")
    ADD_COMM_BUILD_TEST("${name}" "${parent}" "${wrapper}" llviewerprecompiledheaders.cpp)
ENDMACRO(ADD_VIEWER_COMM_BUILD_TEST name parent wrapper)
