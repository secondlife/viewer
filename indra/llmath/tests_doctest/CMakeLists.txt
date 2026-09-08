# Parallel doctest suite for llmath (Issue #4445).
# Keep TUT tests intact until the full migration lands; this target coexists.
include(Doctest)

set(LLMATH_DOCTEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/llmodularmath_test_doctest.cpp
)

add_executable(llmath_doctest
    ${CMAKE_SOURCE_DIR}/test/doctest_main.cpp
    ${CMAKE_SOURCE_DIR}/test/lltest_harness.cpp
    ${LLMATH_DOCTEST_SOURCES}
)

target_include_directories(llmath_doctest
    PRIVATE
        ${DOCTEST_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}/test
        ${CMAKE_SOURCE_DIR}/llcommon
        ${CMAKE_SOURCE_DIR}/llmath
)

target_link_libraries(llmath_doctest
    PRIVATE
        llmath
        llcommon
)

set_target_properties(llmath_doctest PROPERTIES FOLDER "Tests")

add_test(NAME llmath_doctest COMMAND llmath_doctest)
add_dependencies(BUILD_TESTS llmath_doctest)
