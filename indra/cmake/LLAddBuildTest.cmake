
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
        ADD_BUILD_TEST_INTERNAL(${name} ${parent} "${no_common_libraries}" "${no_common_source_files}")
    ENDIF (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/${name}_test.cpp")
ENDMACRO(ADD_BUILD_TEST_NO_COMMON name parent)


MACRO(ADD_BUILD_TEST name parent)
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
            )
        ADD_BUILD_TEST_INTERNAL(${name} ${parent} "${basic_libraries}" "${basic_source_files}")
    ENDIF (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/${name}_test.cpp")
ENDMACRO(ADD_BUILD_TEST name parent)


MACRO(ADD_SIMULATOR_BUILD_TEST name parent)
    SET(sim_source_files
        llsimprecompiledheaders.cpp
        ${name}.cpp
        tests/${name}_test.cpp
        ../test/lltut.cpp
        ../test/test.cpp
        )
    SET(basic_libraries
        ${LLCOMMON_LIBRARIES}
        ${APRUTIL_LIBRARIES}
        ${APR_LIBRARIES}
        ${PTHREAD_LIBRARY}
        ${WINDOWS_LIBRARIES}
        )
    ADD_BUILD_TEST_INTERNAL(${name} ${parent} "${basic_libraries}" "${sim_source_files}")

    if (WINDOWS)
        SET_SOURCE_FILES_PROPERTIES(
            "tests/${name}_test.cpp"
            PROPERTIES
            COMPILE_FLAGS "/Yullsimprecompiledheaders.h"
        )    
    endif (WINDOWS)
ENDMACRO(ADD_SIMULATOR_BUILD_TEST name parent)

MACRO(ADD_BUILD_TEST_INTERNAL name parent libraries source_files)
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
    ADD_CUSTOM_COMMAND(
        OUTPUT ${TEST_OUTPUT}
        COMMAND ${TEST_EXE}
        ARGS --touch=${TEST_OUTPUT} --sourcedir=${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS ${name}_test
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        )
    ADD_CUSTOM_TARGET(${name}_test_ok ALL DEPENDS ${TEST_OUTPUT})
    ADD_DEPENDENCIES(${parent} ${name}_test_ok)

ENDMACRO(ADD_BUILD_TEST_INTERNAL name parent libraries)
