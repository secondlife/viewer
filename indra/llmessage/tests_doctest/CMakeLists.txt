include(Doctest)

add_executable(llmessage_doctest
    ${CMAKE_SOURCE_DIR}/test/doctest_main.cpp
    ${CMAKE_SOURCE_DIR}/test/lltest_harness.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/llxfer_file_test_doctest.cpp
)

target_include_directories(llmessage_doctest
    PRIVATE
        ${CMAKE_SOURCE_DIR}/extern/doctest
        ${CMAKE_SOURCE_DIR}/..
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/test
        ${CMAKE_SOURCE_DIR}/llmessage
        ${CMAKE_SOURCE_DIR}/llcommon
        ${CMAKE_SOURCE_DIR}/llfilesystem
        ${CMAKE_SOURCE_DIR}/llmath
        ${CMAKE_SOURCE_DIR}/llcorehttp
)

target_link_libraries(llmessage_doctest
    PRIVATE
        llmessage
        llfilesystem
        llmath
        llcorehttp
        llcommon
)

add_test(NAME llmessage_doctest COMMAND llmessage_doctest)
