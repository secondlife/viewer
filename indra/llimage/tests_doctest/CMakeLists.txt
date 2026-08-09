include(Doctest)

add_executable(llimage_doctest
    ${CMAKE_SOURCE_DIR}/test/doctest_main.cpp
    ${CMAKE_SOURCE_DIR}/test/lltest_harness.cpp
    ${CMAKE_SOURCE_DIR}/llimage/llimageworker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/llimageworker_test_doctest.cpp
)

target_include_directories(llimage_doctest
    PRIVATE
        ${CMAKE_SOURCE_DIR}/extern/doctest
        ${CMAKE_SOURCE_DIR}/..
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/test
        ${CMAKE_SOURCE_DIR}/llcommon
        ${CMAKE_SOURCE_DIR}/llfilesystem
        ${CMAKE_SOURCE_DIR}/llimage
)

target_link_libraries(llimage_doctest
    PRIVATE
        llcommon
        llmath
        llfilesystem
)

add_test(NAME llimage_doctest COMMAND llimage_doctest)
