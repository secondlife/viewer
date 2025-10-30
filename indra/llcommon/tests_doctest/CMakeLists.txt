file(GLOB LLCOMMON_DOCTEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*_doctest.cpp"
)

add_executable(llcommon_doctest
    ${LLCOMMON_DOCTEST_SOURCES}
    ${CMAKE_SOURCE_DIR}/test/doctest_main.cpp
)

target_include_directories(llcommon_doctest
    PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${DOCTEST_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}/test
        ${CMAKE_SOURCE_DIR}/llcommon
        ${CMAKE_SOURCE_DIR}/llcommon/tests
)

target_link_libraries(llcommon_doctest
    llcommon
)

add_test(NAME llcommon_doctest COMMAND llcommon_doctest)
