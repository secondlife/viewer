include(Doctest)

add_executable(llxml_doctest
    ${CMAKE_SOURCE_DIR}/test/doctest_main.cpp
    ${CMAKE_SOURCE_DIR}/test/lltest_harness.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/llcontrol_test_doctest.cpp
)

target_include_directories(llxml_doctest
    PRIVATE
        ${CMAKE_SOURCE_DIR}/extern/doctest
        ${CMAKE_SOURCE_DIR}/..
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/test
        ${CMAKE_SOURCE_DIR}/llcommon
        ${CMAKE_SOURCE_DIR}/llxml
)

target_link_libraries(llxml_doctest
    PRIVATE
        llxml
        llmath
        llcommon
)

add_test(NAME llxml_doctest COMMAND llxml_doctest)
