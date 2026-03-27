include(Doctest)

add_executable(llinventory_doctest
    ${CMAKE_SOURCE_DIR}/test/doctest_main.cpp
    ${CMAKE_SOURCE_DIR}/test/lltest_harness.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/inventorymisc_test_doctest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/llparcel_test_doctest.cpp
)

target_include_directories(llinventory_doctest
    PRIVATE
        ${CMAKE_SOURCE_DIR}/extern/doctest
        ${CMAKE_SOURCE_DIR}/..
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/test
        ${CMAKE_SOURCE_DIR}/llcommon
        ${CMAKE_SOURCE_DIR}/llmath
        ${CMAKE_SOURCE_DIR}/llfilesystem
        ${CMAKE_SOURCE_DIR}/llcorehttp
        ${CMAKE_SOURCE_DIR}/llmessage
        ${CMAKE_SOURCE_DIR}/llxml
        ${CMAKE_SOURCE_DIR}/llinventory
)

target_link_libraries(llinventory_doctest
    PRIVATE
        llinventory
        llmath
        llfilesystem
        llcorehttp
        llmessage
        llxml
        llcommon
)

add_test(NAME llinventory_doctest COMMAND llinventory_doctest)
