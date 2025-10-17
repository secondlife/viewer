add_executable(llmath_doctest
    ${CMAKE_SOURCE_DIR}/test/doctest_main.cpp
    ${CMAKE_SOURCE_DIR}/llcommon/tests_doctest/win_wstring_shim.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/llquaternion_test_doctest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/llmatrix3_test_doctest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/llmatrix4_test_doctest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/v2math_test_doctest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/v3math_test_doctest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/v4math_test_doctest.cpp
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
