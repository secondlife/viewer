# -*- cmake -*-
#
# Helpers for declaring GoogleTest based unit tests.
#
# GoogleTest itself and the shared llgtest_main library live in indra/gtest.
#
# Usage, from a library's CMakeLists.txt:
#
#   if (LL_TESTS)
#     include(LLAddGTest)
#     ll_add_gtest(llfoo
#         SOURCES      gtests/llfoo_test.cpp gtests/llbar_test.cpp
#         LIBRARIES    llfoo
#         INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
#         )
#   endif ()
#
# This creates the executable target UNIT_TEST_llfoo, matching the
# INTEGRATION_TEST_xxx targets from LLAddBuildTest.cmake.
#
# Options:
#   SOURCES       Test sources (required).
#   LIBRARIES     Libraries/targets the test links against, in addition to
#                 GoogleTest/GoogleMock and llgtest_main.
#   INCLUDE_DIRS  Extra include directories.
#   DEFINITIONS   Extra compile definitions.
#   DEPENDS       Extra targets that must be built before the test runs
#                 (e.g. a shared library that is loaded at runtime).
#   ARGS          Extra command-line arguments passed when ctest runs it.
#   RESOURCE_LOCK Name of a ctest resource lock; tests holding the same lock
#                 never run concurrently under ctest -j (e.g. tests that
#                 compete for the same ports or devices).
#   PER_TEST_PROCESS
#                 Register every TEST()/TEST_F() in SOURCES as its own ctest
#                 test, each run in a separate process.  Use this where tests
#                 can crash or leave process-wide state behind (shutdown
#                 tests, for example): a crash then fails only the test that
#                 crashed instead of hiding every test after it.  Tests are
#                 found by scanning SOURCES at configure time, and editing a
#                 source re-runs configure.  DISABLED_ tests are skipped, and
#                 TEST_P() is not supported in this mode.
#
# Each call creates one executable, UNIT_TEST_<name>, and by default registers
# it as one ctest test of the same name, run through run_build_test.py so
# shared libraries in the staging directories are found.  GoogleTest writes a
# JUnit-style XML report into the build tree for CI: UNIT_TEST_<name>.xml, or
# UNIT_TEST_<name>/<Suite>.<Test>.xml with PER_TEST_PROCESS.

include_guard()

if (NOT LL_TESTS)
  return()
endif ()

include(00-Common)
include(LLTestCommand)
include(LLAddBuildTest) # for SET_TEST_PATH

function(ll_add_gtest name)
  set(target UNIT_TEST_${name})
  cmake_parse_arguments(PARSE_ARGV 1 ARG
      "PER_TEST_PROCESS"
      "RESOURCE_LOCK"
      "SOURCES;LIBRARIES;INCLUDE_DIRS;DEFINITIONS;DEPENDS;ARGS")

  if (NOT ARG_SOURCES)
    message(FATAL_ERROR "ll_add_gtest(${target}): SOURCES is required")
  endif ()
  if (ARG_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "ll_add_gtest(${target}): unknown arguments: ${ARG_UNPARSED_ARGUMENTS}")
  endif ()
  if (NOT TARGET llgtest_main)
    message(FATAL_ERROR "ll_add_gtest(${target}): llgtest_main is not defined; "
                        "indra/gtest must be added before ${CMAKE_CURRENT_SOURCE_DIR}")
  endif ()

  add_executable(${target} ${ARG_SOURCES})

  target_link_libraries(${target} PRIVATE llgtest_main ${ARG_LIBRARIES})
  target_include_directories(${target} PRIVATE ${ARG_INCLUDE_DIRS})
  target_compile_definitions(${target} PRIVATE LL_GTEST=1 ${ARG_DEFINITIONS})

  set_target_properties(${target}
      PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${EXE_STAGING_DIR}"
      FOLDER "Tests"
      )

  if (DARWIN)
    # test binaries always need to be signed for local development
    set_target_properties(${target}
        PROPERTIES
        OSX_ARCHITECTURES ${LL_MACOS_TEST_ARCHITECTURE}
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-"
        BUILD_WITH_INSTALL_RPATH 1
        INSTALL_RPATH "@executable_path/Resources"
        )
  endif ()

  if (ARG_DEPENDS)
    add_dependencies(${target} ${ARG_DEPENDS})
  endif ()

  SET_TEST_PATH(test_lib_path)

  if (ARG_PER_TEST_PROCESS)
    _ll_gtest_list_tests(gtest_names ${ARG_SOURCES})
    if (NOT gtest_names)
      message(FATAL_ERROR "ll_add_gtest(${target}): PER_TEST_PROCESS found no TEST()/TEST_F() in SOURCES")
    endif ()

    set(ctest_names)
    foreach (gtest_name ${gtest_names})
      LL_TEST_COMMAND(test_cmd "${test_lib_path}"
          $<TARGET_FILE:${target}>
          --gtest_filter=${gtest_name}
          --gtest_output=xml:${CMAKE_CURRENT_BINARY_DIR}/${target}/${gtest_name}.xml
          ${ARG_ARGS})
      add_test(
          NAME ${target}.${gtest_name}
          COMMAND ${test_cmd}
          WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
          )
      list(APPEND ctest_names ${target}.${gtest_name})
    endforeach ()
  else ()
    LL_TEST_COMMAND(test_cmd "${test_lib_path}"
        $<TARGET_FILE:${target}>
        --gtest_output=xml:${CMAKE_CURRENT_BINARY_DIR}/${target}.xml
        ${ARG_ARGS})
    add_test(
        NAME ${target}
        COMMAND ${test_cmd}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        )
    set(ctest_names ${target})
  endif ()

  set_tests_properties(${ctest_names} PROPERTIES LABELS "gtest")
  if (ARG_RESOURCE_LOCK)
    set_tests_properties(${ctest_names} PROPERTIES RESOURCE_LOCK "${ARG_RESOURCE_LOCK}")
  endif ()

  add_dependencies(BUILD_TESTS ${target})
endfunction()

# Lists "Suite.Test" for every TEST() and TEST_F() in the given sources
# (relative to the current source dir), skipping DISABLED_ tests.
function(_ll_gtest_list_tests out_var)
  set(names)
  set(ws "[ \t\r\n]*")
  set(id "[A-Za-z_][A-Za-z0-9_]*")
  foreach (source ${ARGN})
    get_filename_component(source_path "${source}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    if (NOT source_path MATCHES "[.](c|cc|cpp|cxx)$")
      continue()
    endif ()

    # Re-run configure when the file changes, so added tests are picked up.
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${source_path}")

    # (Bracket classes rather than backslash escapes for the literal
    # characters, to keep the regexes readable.)
    file(READ "${source_path}" contents)
    set(test_macro "TEST(_F)?[(]${ws}(${id})${ws},${ws}(${id})${ws}[)]")
    string(REGEX MATCHALL "(^|[^A-Za-z0-9_])${test_macro}" matches "${contents}")
    foreach (match ${matches})
      string(REGEX MATCH "${test_macro}" unused "${match}")
      set(name "${CMAKE_MATCH_2}.${CMAKE_MATCH_3}")
      if (NOT name MATCHES "(^|[.])DISABLED_")
        list(APPEND names "${name}")
      endif ()
    endforeach ()
  endforeach ()
  list(REMOVE_DUPLICATES names)
  set(${out_var} ${names} PARENT_SCOPE)
endfunction()
