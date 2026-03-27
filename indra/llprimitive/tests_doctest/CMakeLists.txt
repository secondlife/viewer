include(Doctest)

add_executable(llprimitive_doctest
    ${CMAKE_SOURCE_DIR}/test/doctest_main.cpp
    ${CMAKE_SOURCE_DIR}/test/lltest_harness.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/llmediaentry_test_doctest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/llprimitive_test_doctest.cpp
)

target_include_directories(llprimitive_doctest
    PRIVATE
        ${CMAKE_SOURCE_DIR}/extern/doctest
        ${CMAKE_SOURCE_DIR}/..
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/test
        ${CMAKE_SOURCE_DIR}/llprimitive
        ${CMAKE_SOURCE_DIR}/llcommon
        ${CMAKE_SOURCE_DIR}/llmath
        ${CMAKE_SOURCE_DIR}/llmessage
        ${CMAKE_SOURCE_DIR}/llcorehttp
        ${CMAKE_SOURCE_DIR}/llxml
        ${CMAKE_SOURCE_DIR}/llcharacter
        ${CMAKE_SOURCE_DIR}/llrender
)

target_link_libraries(llprimitive_doctest
    PRIVATE
        llprimitive
        llcommon
        llcorehttp
        llmath
        llmessage
        llrender
        llxml
        llcharacter
)

add_test(NAME llprimitive_doctest COMMAND llprimitive_doctest)
