include(Doctest)

add_executable(llmessage_doctest
    ${CMAKE_SOURCE_DIR}/test/doctest_main.cpp
    ${CMAKE_SOURCE_DIR}/test/lltest_harness.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/llavatarnamecache_test_doctest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/llpartdata_test_doctest.cpp
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

add_executable(lltrustedmessageservice_doctest
    ${CMAKE_SOURCE_DIR}/test/doctest_main.cpp
    ${CMAKE_SOURCE_DIR}/test/lltest_harness.cpp
    ${CMAKE_SOURCE_DIR}/llmessage/lltrustedmessageservice.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/lltrustedmessageservice_test_doctest.cpp
)

target_include_directories(lltrustedmessageservice_doctest
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

target_link_libraries(lltrustedmessageservice_doctest
    PRIVATE
        llfilesystem
        llmath
        llcorehttp
        llcommon
)

add_test(NAME lltrustedmessageservice_doctest COMMAND lltrustedmessageservice_doctest)

add_executable(lltemplatemessagedispatcher_doctest
    ${CMAKE_SOURCE_DIR}/test/doctest_main.cpp
    ${CMAKE_SOURCE_DIR}/test/lltest_harness.cpp
    ${CMAKE_SOURCE_DIR}/llmessage/lltemplatemessagedispatcher.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/lltemplatemessagedispatcher_test_doctest.cpp
)

target_include_directories(lltemplatemessagedispatcher_doctest
    PRIVATE
        ${CMAKE_SOURCE_DIR}/extern/doctest
        ${CMAKE_SOURCE_DIR}/..
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/test
        ${CMAKE_SOURCE_DIR}/llmessage
        ${CMAKE_SOURCE_DIR}/llcommon
        ${CMAKE_SOURCE_DIR}/llmath
        ${CMAKE_SOURCE_DIR}/llcorehttp
        ${CMAKE_SOURCE_DIR}/llmeshoptimizer
)

target_link_libraries(lltemplatemessagedispatcher_doctest
    PRIVATE
        llmath
        llcorehttp
        llcommon
        llmeshoptimizer
)

add_test(NAME lltemplatemessagedispatcher_doctest COMMAND lltemplatemessagedispatcher_doctest)

add_executable(llcoproceduremanager_doctest
    ${CMAKE_SOURCE_DIR}/test/doctest_main.cpp
    ${CMAKE_SOURCE_DIR}/test/lltest_harness.cpp
    ${CMAKE_SOURCE_DIR}/llmessage/llcoproceduremanager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/llcoproceduremanager_test_doctest.cpp
)

target_include_directories(llcoproceduremanager_doctest
    PRIVATE
        ${CMAKE_SOURCE_DIR}/extern/doctest
        ${CMAKE_SOURCE_DIR}/..
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/test
        ${CMAKE_SOURCE_DIR}/llmessage
        ${CMAKE_SOURCE_DIR}/llcommon
        ${CMAKE_SOURCE_DIR}/llmath
        ${CMAKE_SOURCE_DIR}/llcorehttp
        ${CMAKE_SOURCE_DIR}/llmeshoptimizer
)

target_link_libraries(llcoproceduremanager_doctest
    PRIVATE
        llmath
        llcorehttp
        llcommon
        llmeshoptimizer
)

add_test(NAME llcoproceduremanager_doctest COMMAND llcoproceduremanager_doctest)

add_executable(llhost_doctest
    ${CMAKE_SOURCE_DIR}/test/doctest_main.cpp
    ${CMAKE_SOURCE_DIR}/test/lltest_harness.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/llhost_test_doctest.cpp
)

target_include_directories(llhost_doctest
    PRIVATE
        ${CMAKE_SOURCE_DIR}/extern/doctest
        ${CMAKE_SOURCE_DIR}/..
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/test
        ${CMAKE_SOURCE_DIR}/llfilesystem
        ${CMAKE_SOURCE_DIR}/llcommon
        ${CMAKE_SOURCE_DIR}/llmath
        ${CMAKE_SOURCE_DIR}/llmeshoptimizer
        ${CMAKE_SOURCE_DIR}/llcorehttp
        ${CMAKE_SOURCE_DIR}/llmessage
)

target_link_libraries(llhost_doctest
    PRIVATE
        llfilesystem
        llmath
        llcorehttp
        llmessage
        llcommon
        llmeshoptimizer
)

add_test(NAME llhost_doctest COMMAND llhost_doctest)
