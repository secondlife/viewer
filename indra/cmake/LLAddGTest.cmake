# -*- cmake -*-
#
# LL_ADD_GTEST — add a GoogleTest executable for a project.
#
# This is the gtest counterpart to LL_ADD_PROJECT_UNIT_TESTS / LL_ADD_INTEGRATION_TEST.
# Use it for *new* tests written against gtest. Legacy TUT tests stay where they are.
#
# Convention:
#   - Test source lives in `tests_gtest/<name>.cpp` (sibling of legacy `tests/`).
#   - Test executable name: `GTEST_${project}_${name}`.
#   - Each gtest executable is a child of the per-project `${project}_gtests` target.
#   - `${project}_gtests` is added to the build (`ALL`) so tests run as part of normal builds.
#
# Args:
#   project           — owning library (e.g. llmath). Tests link against this lib + llcommon.
#   name              — test name (no extension). Source must be tests_gtest/${name}.cpp.
#   extra_libs        — additional libraries to link.
#
# Source files can use either gtest_main (default) or define their own main().
# We link gtest_main by default; if your test defines main() it will override.
#
# Example, in indra/llmath/CMakeLists.txt:
#   if (LL_TESTS)
#     include(GTest)
#     include(LLAddGTest)
#     LL_ADD_GTEST(llmath llmatrix4 "")
#   endif()

include(GTest)

function(LL_ADD_GTEST project name extra_libs)
  if (NOT LL_TESTS)
    return()
  endif()

  set(target "GTEST_${project}_${name}")
  set(source "tests_gtest/${name}.cpp")

  if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
    message(FATAL_ERROR "LL_ADD_GTEST: missing source ${CMAKE_CURRENT_SOURCE_DIR}/${source}")
  endif()

  add_executable(${target} ${source})

  target_link_libraries(${target}
    PRIVATE
      ${project}
      llcommon
      gtest
      gtest_main
      ${extra_libs}
  )

  # Inherit project's include dirs (mirrors LL_ADD_PROJECT_UNIT_TESTS pattern).
  get_property(_include_dirs TARGET ${project} PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
  if (_include_dirs)
    target_include_directories(${target} PRIVATE ${_include_dirs})
  endif()

  set_target_properties(${target} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${EXE_STAGING_DIR}"
    FOLDER "Tests/${project}_gtests"
  )

  # CRITICAL: force the test exe to be a console subsystem app on Windows.
  # The viewer's global linker flags set /SUBSYSTEM:WINDOWS (it's a GUI app),
  # which would silently swallow gtest's stdout. Mirror what
  # LL_ADD_INTEGRATION_TEST does.
  if (WINDOWS)
    set_target_properties(${target} PROPERTIES
      LINK_FLAGS "/debug /SUBSYSTEM:CONSOLE"
    )
  endif()

  # Run the test as a custom command, touching a marker file on success.
  # Mirrors the TUT test pattern so build failures surface immediately.
  set(_marker "${CMAKE_CURRENT_BINARY_DIR}/${target}_ok.txt")
  add_custom_command(
    OUTPUT  "${_marker}"
    COMMAND $<TARGET_FILE:${target}> --gtest_brief=1
    COMMAND ${CMAKE_COMMAND} -E touch "${_marker}"
    DEPENDS ${target}
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Running ${target}"
    VERBATIM
  )

  # Roll up under a per-project gtest target.
  # NOTE: we deliberately do NOT make ${project} depend on ${_group_target},
  # since the test executable links the project library — that would create
  # a cycle. Tests run as part of the ALL target via the group target itself.
  set(_group_target "${project}_gtests")
  if (NOT TARGET ${_group_target})
    add_custom_target(${_group_target} ALL)
  endif()

  # Add a dummy custom target wrapping the marker so the group can depend on it.
  add_custom_target(${target}_run DEPENDS "${_marker}")
  add_dependencies(${_group_target} ${target}_run)
endfunction()
