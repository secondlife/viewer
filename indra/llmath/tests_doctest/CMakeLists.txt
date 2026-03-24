include(Doctest)

set(LLMATH_DOCTEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/alignment_test_doctest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/llquaternion_test_doctest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/v2math_test_doctest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/v3color_test_doctest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/v3math_test_doctest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/v4math_test_doctest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/v4coloru_test_doctest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/llbbox_test_doctest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/llbboxlocal_test_doctest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/llmodularmath_test_doctest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/m3math_test_doctest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/xform_test_doctest.cpp
)

add_executable(llmath_doctest
    ${CMAKE_SOURCE_DIR}/test/doctest_main.cpp
    ${CMAKE_SOURCE_DIR}/test/lltest_harness.cpp
    ${CMAKE_SOURCE_DIR}/llcommon/tests_doctest/win_wstring_shim.cpp
    ${LLMATH_DOCTEST_SOURCES}
)

target_include_directories(llmath_doctest
    PRIVATE
        ${DOCTEST_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}/..
        ${CMAKE_SOURCE_DIR}/test
        ${PROJECT_SOURCE_DIR}/../test
        ${CMAKE_SOURCE_DIR}/llcommon
        ${CMAKE_SOURCE_DIR}/llmath
        ${CMAKE_SOURCE_DIR}/extern/doctest
        ${CMAKE_SOURCE_DIR}/llcommon/tests_doctest
)

target_link_libraries(llmath_doctest
    PRIVATE
        llmath
        llcommon
)

add_test(NAME llmath_doctest COMMAND llmath_doctest)
