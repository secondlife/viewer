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

add_executable(llagentaccess_doctest
    ${CMAKE_SOURCE_DIR}/test/doctest_main.cpp
    ${CMAKE_SOURCE_DIR}/test/lltest_harness.cpp
    ${CMAKE_SOURCE_DIR}/newview/llagentaccess.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/llagentaccess_test_doctest.cpp
)

target_include_directories(llagentaccess_doctest
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
)

target_link_libraries(llagentaccess_doctest
    PRIVATE
        llcommon
        llmath
        llfilesystem
        llxml
        llmessage
        llcharacter
        llui
        lllogin
        llplugin
        llappearance
)

add_test(NAME llagentaccess_doctest COMMAND llagentaccess_doctest)

add_executable(lldateutil_doctest
    ${CMAKE_SOURCE_DIR}/test/doctest_main.cpp
    ${CMAKE_SOURCE_DIR}/test/lltest_harness.cpp
    ${CMAKE_SOURCE_DIR}/newview/lldateutil.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/lldateutil_test_doctest.cpp
)

target_include_directories(lldateutil_doctest
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
)

target_link_libraries(lldateutil_doctest
    PRIVATE
        llcommon
        llfilesystem
        llxml
        llmessage
        llcharacter
        llui
        lllogin
        llplugin
        llappearance
)

add_test(NAME lldateutil_doctest COMMAND lldateutil_doctest)

add_executable(llversioninfo_doctest
    ${CMAKE_SOURCE_DIR}/test/doctest_main.cpp
    ${CMAKE_SOURCE_DIR}/test/lltest_harness.cpp
    ${CMAKE_SOURCE_DIR}/newview/llversioninfo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/llversioninfo_test_doctest.cpp
)

target_include_directories(llversioninfo_doctest
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
)

target_link_libraries(llversioninfo_doctest
    PRIVATE
        llcommon
        llfilesystem
        llxml
        llmessage
        llcharacter
        llui
        lllogin
        llplugin
        llappearance
)

target_compile_definitions(llversioninfo_doctest
    PRIVATE
        ${VIEWER_CHANNEL_VERSION_DEFINES}
)

add_test(NAME llversioninfo_doctest COMMAND llversioninfo_doctest)

add_executable(llworldmipmap_doctest
    ${CMAKE_SOURCE_DIR}/test/doctest_main.cpp
    ${CMAKE_SOURCE_DIR}/test/lltest_harness.cpp
    ${CMAKE_SOURCE_DIR}/newview/llworldmipmap.cpp
    ${CMAKE_SOURCE_DIR}/newview/tests/llviewertexture_stub.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/llworldmipmap_test_doctest.cpp
)

target_include_directories(llworldmipmap_doctest
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
)

target_link_libraries(llworldmipmap_doctest
    PRIVATE
        llcommon
        llfilesystem
        llxml
        llmessage
        llcharacter
        llui
        lllogin
        llplugin
        llappearance
)

add_test(NAME llworldmipmap_doctest COMMAND llworldmipmap_doctest)
