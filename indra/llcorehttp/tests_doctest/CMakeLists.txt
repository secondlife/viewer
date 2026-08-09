include(Doctest)

add_executable(llcorehttp_doctest
    ${CMAKE_SOURCE_DIR}/test/doctest_main.cpp
    ${CMAKE_SOURCE_DIR}/test/lltest_harness.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/http_fakes.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/bufferarray_test_doctest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/httpheaders_test_doctest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/httpoperation_test_doctest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/httprequestqueue_test_doctest.cpp
)

target_include_directories(llcorehttp_doctest
    PRIVATE
        ${DOCTEST_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/test
        ${CMAKE_SOURCE_DIR}/extern/doctest
        ${CMAKE_SOURCE_DIR}/llcorehttp
        ${CMAKE_SOURCE_DIR}/llcommon
)

target_link_libraries(llcorehttp_doctest
    PRIVATE
        llcorehttp
        llmessage
        llcommon
)

add_test(NAME llcorehttp_doctest COMMAND llcorehttp_doctest)
