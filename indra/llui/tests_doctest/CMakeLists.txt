include(Doctest)

set(llui_doctest_include_dirs
    ${CMAKE_SOURCE_DIR}/extern/doctest
    ${CMAKE_SOURCE_DIR}/..
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/test
    ${CMAKE_SOURCE_DIR}/llui
    ${CMAKE_SOURCE_DIR}/llrender
    ${CMAKE_SOURCE_DIR}/llcommon
    ${CMAKE_SOURCE_DIR}/llimage
    ${CMAKE_SOURCE_DIR}/llfilesystem
    ${CMAKE_SOURCE_DIR}/llmath
    ${CMAKE_SOURCE_DIR}/llxml
    ${CMAKE_SOURCE_DIR}/llwindow
    ${CMAKE_SOURCE_DIR}/llinventory
    ${CMAKE_SOURCE_DIR}/llmessage
    ${CMAKE_SOURCE_DIR}/llcorehttp
)

add_executable(llurlmatch_doctest
    ${CMAKE_SOURCE_DIR}/test/doctest_main.cpp
    ${CMAKE_SOURCE_DIR}/test/lltest_harness.cpp
    ${CMAKE_SOURCE_DIR}/llui/llurlmatch.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/llurlmatch_test_doctest.cpp
)

target_include_directories(llurlmatch_doctest PRIVATE ${llui_doctest_include_dirs})

target_link_libraries(llurlmatch_doctest
    PRIVATE
        llcommon
        llmath
        llmessage
        llcorehttp
        llxml
        llrender
        ll::hunspell
)

add_test(NAME llurlmatch_doctest COMMAND llurlmatch_doctest)

if(NOT LINUX)
    add_executable(llurlentry_doctest
        ${CMAKE_SOURCE_DIR}/test/doctest_main.cpp
        ${CMAKE_SOURCE_DIR}/test/lltest_harness.cpp
        ${CMAKE_SOURCE_DIR}/llui/llurlentry.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/llurlentry_test_doctest.cpp
    )

    target_include_directories(llurlentry_doctest PRIVATE ${llui_doctest_include_dirs})

    target_link_libraries(llurlentry_doctest
        PRIVATE
            llui
            llmessage
            llcorehttp
            llxml
            llrender
            llcommon
            ll::hunspell
    )

    add_test(NAME llurlentry_doctest COMMAND llurlentry_doctest)
endif()
