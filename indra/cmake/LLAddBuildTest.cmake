# -*- cmake -*-

MACRO(LL_ADD_PROJECT_UNIT_TESTS project sources)
  # Given a project name and a list of sourcefiles (with optional properties on each),
  # add targets to build and run the tests specified.
  # ASSUMPTIONS:
  # * this macro is being executed in the project file that is passed in
  # * current working SOURCE dir is that project dir
  # * there is a subfolder tests/ with test code corresponding to the filenames passed in
  # * properties for each sourcefile passed in indicate what libs to link that file with (MAKE NO ASSUMPTIONS ASIDE FROM TUT)
  #
  # More info and examples at: https://wiki.secondlife.com/wiki/How_to_add_unit_tests_to_indra_code
  #
  # WARNING: do NOT modify this code without working with poppy or daveh -
  # there is another branch that will conflict heavily with any changes here.

  IF(LL_TEST_VERBOSE)
    MESSAGE("LL_ADD_PROJECT_UNIT_TESTS UNITTEST_PROJECT_${project} sources: ${sources}")
  ENDIF(LL_TEST_VERBOSE)

  # Start with the header and project-wide setup before making targets
  #project(UNITTEST_PROJECT_${project})
  # Setup includes, paths, etc
  SET(alltest_SOURCE_FILES
    ${CMAKE_SOURCE_DIR}/test/test.cpp
    )
  SET(alltest_DEP_TARGETS
    llcommon
    llmath
    )
  SET(alltest_INCLUDE_DIRS
    ${LLMATH_INCLUDE_DIRS}
    ${LLCOMMON_INCLUDE_DIRS}
    ${LIBS_OPEN_DIR}/test
    )
  SET(alltest_LIBRARIES
    ${PTHREAD_LIBRARY}
    ${WINDOWS_LIBRARIES}
    )
  # Headers, for convenience in targets.
  SET(alltest_HEADER_FILES
    ${CMAKE_SOURCE_DIR}/test/test.h
    )

  # start the source test executable definitions
  SET(${project}_TEST_OUTPUT "")
  FOREACH (source ${sources})
    STRING( REGEX REPLACE "(.*)\\.[^.]+$" "\\1" name ${source} )
    STRING( REGEX REPLACE ".*\\.([^.]+)$" "\\1" extension ${source} )
    IF(LL_TEST_VERBOSE)
      MESSAGE("LL_ADD_PROJECT_UNIT_TESTS UNITTEST_PROJECT_${project} individual source: ${source} (${name}.${extension})")
    ENDIF(LL_TEST_VERBOSE)

    #
    # Per-codefile additional / external source, header, and include dir property extraction
    #
    # Source
    GET_SOURCE_FILE_PROPERTY(${name}_test_additional_SOURCE_FILES ${source} LL_TEST_ADDITIONAL_SOURCE_FILES)
    IF(${name}_test_additional_SOURCE_FILES MATCHES NOTFOUND)
      SET(${name}_test_additional_SOURCE_FILES "")
    ENDIF(${name}_test_additional_SOURCE_FILES MATCHES NOTFOUND)
    SET(${name}_test_SOURCE_FILES ${source} tests/${name}_test.${extension} ${alltest_SOURCE_FILES} ${${name}_test_additional_SOURCE_FILES} )
    IF(LL_TEST_VERBOSE)
      MESSAGE("LL_ADD_PROJECT_UNIT_TESTS ${name}_test_SOURCE_FILES ${${name}_test_SOURCE_FILES}")
    ENDIF(LL_TEST_VERBOSE)
    # Headers
    GET_SOURCE_FILE_PROPERTY(${name}_test_additional_HEADER_FILES ${source} LL_TEST_ADDITIONAL_HEADER_FILES)
    IF(${name}_test_additional_HEADER_FILES MATCHES NOTFOUND)
      SET(${name}_test_additional_HEADER_FILES "")
    ENDIF(${name}_test_additional_HEADER_FILES MATCHES NOTFOUND)
    SET(${name}_test_HEADER_FILES ${name}.h ${${name}_test_additional_HEADER_FILES})
    set_source_files_properties(${${name}_test_HEADER_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)
    LIST(APPEND ${name}_test_SOURCE_FILES ${${name}_test_HEADER_FILES})
    IF(LL_TEST_VERBOSE)
      MESSAGE("LL_ADD_PROJECT_UNIT_TESTS ${name}_test_HEADER_FILES ${${name}_test_HEADER_FILES}")
    ENDIF(LL_TEST_VERBOSE)
    # Include dirs
    GET_SOURCE_FILE_PROPERTY(${name}_test_additional_INCLUDE_DIRS ${source} LL_TEST_ADDITIONAL_INCLUDE_DIRS)
    IF(${name}_test_additional_INCLUDE_DIRS MATCHES NOTFOUND)
      SET(${name}_test_additional_INCLUDE_DIRS "")
    ENDIF(${name}_test_additional_INCLUDE_DIRS MATCHES NOTFOUND)
    INCLUDE_DIRECTORIES(${alltest_INCLUDE_DIRS} ${name}_test_additional_INCLUDE_DIRS )
    IF(LL_TEST_VERBOSE)
      MESSAGE("LL_ADD_PROJECT_UNIT_TESTS ${name}_test_additional_INCLUDE_DIRS ${${name}_test_additional_INCLUDE_DIRS}")
    ENDIF(LL_TEST_VERBOSE)

    # Setup target
    ADD_EXECUTABLE(PROJECT_${project}_TEST_${name} ${${name}_test_SOURCE_FILES})

    #
    # Per-codefile additional / external project dep and lib dep property extraction
    #
    # WARNING: it's REALLY IMPORTANT to not mix these. I guarantee it will not work in the future. + poppy 2009-04-19
    # Projects
    GET_SOURCE_FILE_PROPERTY(${name}_test_additional_PROJECTS ${source} LL_TEST_ADDITIONAL_PROJECTS)
    IF(${name}_test_additional_PROJECTS MATCHES NOTFOUND)
      SET(${name}_test_additional_PROJECTS "")
    ENDIF(${name}_test_additional_PROJECTS MATCHES NOTFOUND)
    # Libraries
    GET_SOURCE_FILE_PROPERTY(${name}_test_additional_LIBRARIES ${source} LL_TEST_ADDITIONAL_LIBRARIES)
    IF(${name}_test_additional_LIBRARIES MATCHES NOTFOUND)
      SET(${name}_test_additional_LIBRARIES "")
    ENDIF(${name}_test_additional_LIBRARIES MATCHES NOTFOUND)
    IF(LL_TEST_VERBOSE)
      MESSAGE("LL_ADD_PROJECT_UNIT_TESTS ${name}_test_additional_PROJECTS ${${name}_test_additional_PROJECTS}")
      MESSAGE("LL_ADD_PROJECT_UNIT_TESTS ${name}_test_additional_LIBRARIES ${${name}_test_additional_LIBRARIES}")
    ENDIF(LL_TEST_VERBOSE)
    # Add to project
    TARGET_LINK_LIBRARIES(PROJECT_${project}_TEST_${name} ${alltest_LIBRARIES} ${alltest_DEP_TARGETS} ${${name}_test_additional_PROJECTS} ${${name}_test_additional_LIBRARIES} )
    
    #
    # Setup test targets
    #
    GET_TARGET_PROPERTY(TEST_EXE PROJECT_${project}_TEST_${name} LOCATION)
    SET(TEST_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/PROJECT_${project}_TEST_${name}_ok.txt)
    SET(TEST_CMD ${TEST_EXE} --touch=${TEST_OUTPUT} --sourcedir=${CMAKE_CURRENT_SOURCE_DIR})
    # daveh - what configuration does this use? Debug? it's cmake-time, not build time. + poppy 2009-04-19
    IF(LL_TEST_VERBOSE)
      MESSAGE(STATUS "LL_ADD_PROJECT_UNIT_TESTS ${name} test_cmd  = ${TEST_CMD}")
    ENDIF(LL_TEST_VERBOSE)
	
	IF(WINDOWS)
	  set(LD_LIBRARY_PATH ${SHARED_LIB_STAGING_DIR}/${CMAKE_CFG_INTDIR})
	ELSE(WINDOWS)
	  set(LD_LIBRARY_PATH ${ARCH_PREBUILT_DIRS}:/usr/lib)
	ENDIF(WINDOWS)

    SET(TEST_SCRIPT_CMD 
      ${CMAKE_COMMAND} 
      -DLD_LIBRARY_PATH=${LD_LIBRARY_PATH}
      -DTEST_CMD:STRING="${TEST_CMD}" 
      -P ${CMAKE_SOURCE_DIR}/cmake/RunBuildTest.cmake
      )
    IF(LL_TEST_VERBOSE)
      MESSAGE(STATUS "LL_ADD_PROJECT_UNIT_TESTS ${name} test_script  = ${TEST_SCRIPT_CMD}")
    ENDIF(LL_TEST_VERBOSE)
    # Add test 
    ADD_CUSTOM_COMMAND(
        OUTPUT ${TEST_OUTPUT}
        COMMAND ${TEST_SCRIPT_CMD}
        DEPENDS PROJECT_${project}_TEST_${name}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        )
    # Why not add custom target and add POST_BUILD command?
    # Slightly less uncertain behavior
    # (OUTPUT commands run non-deterministically AFAIK) + poppy 2009-04-19
    # > I did not use a post build step as I could not make it notify of a 
    # > failure after the first time you build and fail a test. - daveh 2009-04-20
    LIST(APPEND ${project}_TEST_OUTPUT ${TEST_OUTPUT})
  ENDFOREACH (source)

  # Add the test runner target per-project
  # (replaces old _test_ok targets all over the place)
  ADD_CUSTOM_TARGET(${project}_tests ALL DEPENDS ${${project}_TEST_OUTPUT})
  ADD_DEPENDENCIES(${project} ${project}_tests)
ENDMACRO(LL_ADD_PROJECT_UNIT_TESTS)

