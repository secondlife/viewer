include(Doctest)

add_executable(cppfeatures_doctest
    ${CMAKE_SOURCE_DIR}/test/doctest_main.cpp
    ${CMAKE_SOURCE_DIR}/test/lltest_harness.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cppfeatures_test_doctest.cpp
)

target_include_directories(cppfeatures_doctest
    PRIVATE
        ${CMAKE_SOURCE_DIR}/extern/doctest
        ${CMAKE_SOURCE_DIR}/..
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/test
        ${CMAKE_SOURCE_DIR}/newview
        ${CMAKE_SOURCE_DIR}/llcommon
        ${CMAKE_SOURCE_DIR}/llmath
        ${CMAKE_SOURCE_DIR}/llmeshoptimizer
        ${CMAKE_SOURCE_DIR}/llfilesystem
        ${CMAKE_SOURCE_DIR}/llxml
        ${CMAKE_SOURCE_DIR}/llmessage
        ${CMAKE_SOURCE_DIR}/llcorehttp
        ${CMAKE_SOURCE_DIR}/llcharacter
        ${CMAKE_SOURCE_DIR}/llui
        ${CMAKE_SOURCE_DIR}/llrender
        ${CMAKE_SOURCE_DIR}/llimage
        ${CMAKE_SOURCE_DIR}/llwindow
        ${CMAKE_SOURCE_DIR}/llinventory
        ${CMAKE_SOURCE_DIR}/viewer_components/login
        ${CMAKE_SOURCE_DIR}/llplugin
        ${CMAKE_SOURCE_DIR}/llappearance
        ${CMAKE_SOURCE_DIR}/llprimitive
        ${CMAKE_SOURCE_DIR}/llphysicsextensionsos
)

target_link_libraries(cppfeatures_doctest
    PRIVATE
        llfilesystem
        llmath
        llcommon
        llmessage
        llcorehttp
        llxml
        llui
        llplugin
        llappearance
        lllogin
        llprimitive
)

add_test(NAME cppfeatures_doctest COMMAND cppfeatures_doctest)
