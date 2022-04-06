# -*- cmake -*-

project (lltest)

include(00-Common)
include(LLCommon)
include(LLCoreHttp)
include(Linking)
include(Tut)
include(LLAddBuildTest)
include(bugsplat)
include(GoogleMock)

set(test_SOURCE_FILES
    io.cpp
    llapp_tut.cpp
    llblowfish_tut.cpp
    llbuffer_tut.cpp
    lldoubledispatch_tut.cpp
    llevents_tut.cpp
    llhttpdate_tut.cpp
    llhttpnode_tut.cpp
    lliohttpserver_tut.cpp
    llmessageconfig_tut.cpp
    llpermissions_tut.cpp
    llpipeutil.cpp
    llsaleinfo_tut.cpp
    llsdmessagebuilder_tut.cpp
    llsdmessagereader_tut.cpp
    llsd_new_tut.cpp
    llsdutil_tut.cpp
    llservicebuilder_tut.cpp
    llstreamtools_tut.cpp
    lltemplatemessagebuilder_tut.cpp
    lltut.cpp
    message_tut.cpp
    test.cpp
    )

set(test_HEADER_FILES
    CMakeLists.txt

    debug.h
    llpipeutil.h
    llsdtraits.h
    lltut.h
    sync.h
    )

if (NOT WINDOWS)
  list(APPEND test_SOURCE_FILES
       llmessagetemplateparser_tut.cpp
       )
endif (NOT WINDOWS)

set_source_files_properties(${test_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND test_SOURCE_FILES ${test_HEADER_FILES})

add_executable(lltest ${test_SOURCE_FILES})

if (USE_BUGSPLAT)
  set_target_properties(lltest PROPERTIES COMPILE_DEFINITIONS "${BUGSPLAT_DEFINE}")
endif (USE_BUGSPLAT)

target_link_libraries(lltest
        ${LEGACY_STDIO_LIBS}
        ${LLDATABASE_LIBRARIES}
        llinventory
        llmessage
        llmath
        llfilesystem
        llxml
        llcommon
        llcorehttp
        ${WINDOWS_LIBRARIES}
        ${DL_LIBRARY}
        )

if (WINDOWS)
  set_target_properties(lltest
          PROPERTIES 
          LINK_FLAGS "/NODEFAULTLIB:LIBCMT"
          LINK_FLAGS_DEBUG "/NODEFAULTLIB:\"LIBCMT;LIBCMTD;MSVCRT\""
          )
endif (WINDOWS)

set(TEST_EXE $<TARGET_FILE:lltest>)

SET_TEST_PATH(DYLD_LIBRARY_PATH)

LL_TEST_COMMAND(command 
  "${DYLD_LIBRARY_PATH}"
  "${TEST_EXE}"
  "--output=${CMAKE_CURRENT_BINARY_DIR}/cpp_test_results.txt" 
  "--touch=${CMAKE_CURRENT_BINARY_DIR}/cpp_tests_ok.txt")

ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/cpp_tests_ok.txt
  COMMAND ${command}
  DEPENDS lltest
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "C++ unit tests"
  )

set(test_results ${CMAKE_CURRENT_BINARY_DIR}/cpp_tests_ok.txt)

# This should cause the test executable to be built, but not 
# run if LL_TESTS is disabled. This will hopefully keep the
# tests up to date with any code changes changes even if 
# developers choose to disable LL_TESTS.
if (LL_TESTS)  
    add_custom_target(tests_ok ALL DEPENDS ${test_results})
endif (LL_TESTS)
