include(Doctest)

add_executable(llfilesystem_doctest
    ${CMAKE_SOURCE_DIR}/test/doctest_main.cpp
    ${CMAKE_SOURCE_DIR}/test/lltest_harness.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/lldiriterator_test_doctest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/lldir_test_doctest.cpp
)

target_include_directories(llfilesystem_doctest
    PRIVATE
        ${CMAKE_SOURCE_DIR}/extern/doctest
        ${CMAKE_SOURCE_DIR}/..
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/test
        ${CMAKE_SOURCE_DIR}/llcommon
        ${CMAKE_SOURCE_DIR}/llfilesystem
)

target_link_libraries(llfilesystem_doctest
    PRIVATE
        llfilesystem
        llmath
        llcommon
)

add_test(NAME llfilesystem_doctest COMMAND llfilesystem_doctest)
