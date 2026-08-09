include(Doctest)

add_executable(llcharacter_doctest
    ${CMAKE_SOURCE_DIR}/test/doctest_main.cpp
    ${CMAKE_SOURCE_DIR}/test/lltest_harness.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/lljoint_test_doctest.cpp
)

target_include_directories(llcharacter_doctest
    PRIVATE
        ${CMAKE_SOURCE_DIR}/extern/doctest
        ${CMAKE_SOURCE_DIR}/..
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/test
        ${CMAKE_SOURCE_DIR}/llcharacter
        ${CMAKE_SOURCE_DIR}/llmath
)

target_link_libraries(llcharacter_doctest
    PRIVATE
        llcharacter
)

add_test(NAME llcharacter_doctest COMMAND llcharacter_doctest)
