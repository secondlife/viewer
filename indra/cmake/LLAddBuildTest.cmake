# -*- cmake -*-

include_guard()

if( NOT LL_TESTS )
  return()
endif()

include(00-Common)
include(LLTestCommand)
include(bugsplat)
include(Tut)

#*****************************************************************************
#   LL_ADD_PROJECT_UNIT_TESTS
#*****************************************************************************
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

  if(LL_TEST_VERBOSE)
    message("LL_ADD_PROJECT_UNIT_TESTS UNITTEST_PROJECT_${project} sources: ${sources}")
  endif()

  # Start with the header and project-wide setup before making targets
  #project(UNITTEST_PROJECT_${project})
  # Setup includes, paths, etc
  set(alltest_SOURCE_FILES
          ${CMAKE_SOURCE_DIR}/test/test.cpp
          ${CMAKE_SOURCE_DIR}/test/lltut.cpp
          )
  set(alltest_DEP_TARGETS
          # needed by the test harness itself
          llcommon
          )

  set(alltest_LIBRARIES
          llcommon
          )
  if(NOT "${project}" STREQUAL "llmath")
    # add llmath as a dep unless the tested module *is* llmath!
    list(APPEND alltest_DEP_TARGETS llmath)
    list(APPEND alltest_LIBRARIES llmath )
  endif()

  # Headers, for convenience in targets.
  set(alltest_HEADER_FILES ${CMAKE_SOURCE_DIR}/test/test.h)

  # start the source test executable definitions
  set(${project}_TEST_OUTPUT "")
  foreach (source ${sources})
    string( REGEX REPLACE "(.*)\\.[^.]+$" "\\1" name ${source} )
    string( REGEX REPLACE ".*\\.([^.]+)$" "\\1" extension ${source} )
    if(LL_TEST_VERBOSE)
      message("LL_ADD_PROJECT_UNIT_TESTS UNITTEST_PROJECT_${project} individual source: ${source} (${name}.${extension})")
    endif()

    #
    # Per-codefile additional / external source, header, and include dir property extraction
    #
    # Source
    GET_OPT_SOURCE_FILE_PROPERTY(${name}_test_additional_SOURCE_FILES ${source} LL_TEST_ADDITIONAL_SOURCE_FILES)
    set(${name}_test_SOURCE_FILES
            ${source}
            tests/${name}_test.${extension}
            ${alltest_SOURCE_FILES}
            ${${name}_test_additional_SOURCE_FILES} )
    if(LL_TEST_VERBOSE)
      message("LL_ADD_PROJECT_UNIT_TESTS ${name}_test_SOURCE_FILES ${${name}_test_SOURCE_FILES}")
    endif()

    # Headers
    GET_OPT_SOURCE_FILE_PROPERTY(${name}_test_additional_HEADER_FILES ${source} LL_TEST_ADDITIONAL_HEADER_FILES)
    set(${name}_test_HEADER_FILES ${name}.h ${${name}_test_additional_HEADER_FILES})
    list(APPEND ${name}_test_SOURCE_FILES ${${name}_test_HEADER_FILES})
    if(LL_TEST_VERBOSE)
      message("LL_ADD_PROJECT_UNIT_TESTS ${name}_test_HEADER_FILES ${${name}_test_HEADER_FILES}")
    endif()

    # Setup target
    add_executable(PROJECT_${project}_TEST_${name} ${${name}_test_SOURCE_FILES})

    # Cannot declare a dependency on ${project} because the executable create above will later declare
    # add_dependencies( ${project} ${project}_tests)
    # as such grab ${project}'s interface include dirs and inject them here
    get_property( ${name}_test_additional_INCLUDE_DIRS TARGET ${project} PROPERTY INTERFACE_INCLUDE_DIRECTORIES )
    target_include_directories (PROJECT_${project}_TEST_${name} PRIVATE ${${name}_test_additional_INCLUDE_DIRS} )

    GET_OPT_SOURCE_FILE_PROPERTY(${name}_test_additional_INCLUDE_DIRS ${source} LL_TEST_ADDITIONAL_INCLUDE_DIRS)
    target_include_directories (PROJECT_${project}_TEST_${name} PRIVATE ${${name}_test_additional_INCLUDE_DIRS} )

    target_include_directories (PROJECT_${project}_TEST_${name} PRIVATE ${LIBS_OPEN_DIR}/test )

    set_target_properties(PROJECT_${project}_TEST_${name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${EXE_STAGING_DIR}")

    #
    # Per-codefile additional / external project dep and lib dep property extraction
    #
    # WARNING: it's REALLY IMPORTANT to not mix these. I guarantee it will not work in the future. + poppy 2009-04-19
    # Projects
    GET_OPT_SOURCE_FILE_PROPERTY(${name}_test_additional_PROJECTS ${source} LL_TEST_ADDITIONAL_PROJECTS)
    # Libraries
    GET_OPT_SOURCE_FILE_PROPERTY(${name}_test_additional_LIBRARIES ${source} LL_TEST_ADDITIONAL_LIBRARIES)

    if(LL_TEST_VERBOSE)
      message("LL_ADD_PROJECT_UNIT_TESTS ${name}_test_additional_PROJECTS ${${name}_test_additional_PROJECTS}")
      message("LL_ADD_PROJECT_UNIT_TESTS ${name}_test_additional_LIBRARIES ${${name}_test_additional_LIBRARIES}")
    endif()

    # Add to project
    target_link_libraries(PROJECT_${project}_TEST_${name} ${alltest_LIBRARIES} ${${name}_test_additional_PROJECTS} ${${name}_test_additional_LIBRARIES} )
    add_dependencies( PROJECT_${project}_TEST_${name} ${alltest_DEP_TARGETS})
    # Compile-time Definitions
    GET_OPT_SOURCE_FILE_PROPERTY(${name}_test_additional_CFLAGS ${source} LL_TEST_ADDITIONAL_CFLAGS)
    set_target_properties(PROJECT_${project}_TEST_${name}
            PROPERTIES
            COMPILE_FLAGS "${${name}_test_additional_CFLAGS}"
            COMPILE_DEFINITIONS "LL_TEST=${name};LL_TEST_${name}")
    if(LL_TEST_VERBOSE)
      message("LL_ADD_PROJECT_UNIT_TESTS ${name}_test_additional_CFLAGS ${${name}_test_additional_CFLAGS}")
    endif()

    if (DARWIN)
      # test binaries always need to be signed for local development
      set_target_properties(PROJECT_${project}_TEST_${name}
          PROPERTIES
              XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-")
    endif ()

    #
    # Setup test targets
    #
    set(TEST_EXE $<TARGET_FILE:PROJECT_${project}_TEST_${name}>)
    set(TEST_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/PROJECT_${project}_TEST_${name}_ok.txt)
    set(TEST_CMD ${TEST_EXE} --touch=${TEST_OUTPUT} --sourcedir=${CMAKE_CURRENT_SOURCE_DIR})

    # daveh - what configuration does this use? Debug? it's cmake-time, not build time. + poppy 2009-04-19
    if(LL_TEST_VERBOSE)
      message(STATUS "LL_ADD_PROJECT_UNIT_TESTS ${name} test_cmd  = ${TEST_CMD}")
    endif()

    SET_TEST_PATH(LD_LIBRARY_PATH)
    LL_TEST_COMMAND(TEST_SCRIPT_CMD "${LD_LIBRARY_PATH}" ${TEST_CMD})
    if(LL_TEST_VERBOSE)
      message(STATUS "LL_ADD_PROJECT_UNIT_TESTS ${name} test_script  = ${TEST_SCRIPT_CMD}")
    endif()

    # Add test
    add_custom_command(
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
    list(APPEND ${project}_TEST_OUTPUT ${TEST_OUTPUT})
  endforeach (source)

  # Add the test runner target per-project
  # (replaces old _test_ok targets all over the place)
  add_custom_target(${project}_tests ALL DEPENDS ${${project}_TEST_OUTPUT})
  add_dependencies(${project} ${project}_tests)
ENDMACRO(LL_ADD_PROJECT_UNIT_TESTS)

#*****************************************************************************
#   GET_OPT_SOURCE_FILE_PROPERTY
#*****************************************************************************
MACRO(GET_OPT_SOURCE_FILE_PROPERTY var filename property)
  get_source_file_property(${var} "${filename}" "${property}")
  if("${${var}}" MATCHES NOTFOUND)
    set(${var} "")
  endif()
ENDMACRO(GET_OPT_SOURCE_FILE_PROPERTY)

#*****************************************************************************
#   LL_ADD_INTEGRATION_TEST
#*****************************************************************************
FUNCTION(LL_ADD_INTEGRATION_TEST
        testname
        additional_source_files
        library_dependencies
        # variable args
        )
  if(TEST_DEBUG)
    message(STATUS "Adding INTEGRATION_TEST_${testname} - debug output is on")
  endif()

  set(source_files
          tests/${testname}_test.cpp
          ${CMAKE_SOURCE_DIR}/test/test.cpp
          ${CMAKE_SOURCE_DIR}/test/lltut.cpp
          ${additional_source_files}
          )

  set(libraries
          ${library_dependencies}
          )

  # Add test executable build target
  if(TEST_DEBUG)
    message(STATUS "ADD_EXECUTABLE(INTEGRATION_TEST_${testname} ${source_files})")
  endif()

  add_executable(INTEGRATION_TEST_${testname} ${source_files})
  set_target_properties(INTEGRATION_TEST_${testname}
          PROPERTIES
          RUNTIME_OUTPUT_DIRECTORY "${EXE_STAGING_DIR}"
          COMPILE_DEFINITIONS "LL_TEST=${testname};LL_TEST_${testname}"
          )

  # The following was copied to llcorehttp/CMakeLists.txt's texture_load target.
  # Any changes made here should be replicated there.
  if (WINDOWS)
    set_target_properties(INTEGRATION_TEST_${testname}
            PROPERTIES
            LINK_FLAGS "/debug /NODEFAULTLIB:LIBCMT /SUBSYSTEM:CONSOLE"
            )
  endif ()

  if (DARWIN)
    # test binaries always need to be signed for local development
    set_target_properties(INTEGRATION_TEST_${testname}
            PROPERTIES
            XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-")
  endif ()

  # Add link deps to the executable
  if(TEST_DEBUG)
    message(STATUS "TARGET_LINK_LIBRARIES(INTEGRATION_TEST_${testname} ${libraries})")
  endif()

  target_link_libraries(INTEGRATION_TEST_${testname} ${libraries})
  target_include_directories (INTEGRATION_TEST_${testname} PRIVATE ${LIBS_OPEN_DIR}/test )

  # Create the test running command
  set(test_command ${ARGN})
  set(TEST_EXE $<TARGET_FILE:INTEGRATION_TEST_${testname}>)
  list(FIND test_command "{}" test_exe_pos)
  if(test_exe_pos LESS 0)
    # The {} marker means "the full pathname of the test executable."
    # test_exe_pos -1 means we didn't find it -- so append the test executable
    # name to $ARGN, the variable part of the arg list. This is convenient
    # shorthand for both straightforward execution of the test program (empty
    # $ARGN) and for running a "wrapper" program of some kind accepting the
    # pathname of the test program as the last of its args. You need specify
    # {} only if the test program's pathname isn't the last argument in the
    # desired command line.
    list(APPEND test_command "${TEST_EXE}")
  else (test_exe_pos LESS 0)
    # Found {} marker at test_exe_pos. Remove the {}...
    list(REMOVE_AT test_command test_exe_pos)
    # ...and replace it with the actual name of the test executable.
    list(INSERT test_command test_exe_pos "${TEST_EXE}")
  endif()

  SET_TEST_PATH(LD_LIBRARY_PATH)
  LL_TEST_COMMAND(TEST_SCRIPT_CMD "${LD_LIBRARY_PATH}" ${test_command})

  if(TEST_DEBUG)
    message(STATUS "TEST_SCRIPT_CMD: ${TEST_SCRIPT_CMD}")
  endif()

  add_custom_command(
          TARGET INTEGRATION_TEST_${testname}
          POST_BUILD
          COMMAND ${TEST_SCRIPT_CMD}
  )

  # Use CTEST? Not sure how to yet...
  # ADD_TEST(INTEGRATION_TEST_RUNNER_${testname} ${TEST_SCRIPT_CMD})

ENDFUNCTION(LL_ADD_INTEGRATION_TEST)

#*****************************************************************************
#   SET_TEST_PATH
#*****************************************************************************
MACRO(SET_TEST_PATH LISTVAR)
  IF(WINDOWS)
    # We typically build/package only Release variants of third-party
    # libraries, so append the Release staging dir in case the library being
    # sought doesn't have a debug variant.
    set(${LISTVAR} ${SHARED_LIB_STAGING_DIR} ${SHARED_LIB_STAGING_DIR}/Release)
  ELSEIF(DARWIN)
    # We typically build/package only Release variants of third-party
    # libraries, so append the Release staging dir in case the library being
    # sought doesn't have a debug variant.
    set(${LISTVAR} ${SHARED_LIB_STAGING_DIR} ${SHARED_LIB_STAGING_DIR}/Release/Resources /usr/lib)
  ELSE(WINDOWS)
    # Linux uses a single staging directory anyway.
    set(${LISTVAR} ${SHARED_LIB_STAGING_DIR} /usr/lib)
  ENDIF(WINDOWS)
ENDMACRO(SET_TEST_PATH)
