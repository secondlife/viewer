# -*- cmake -*-

project (test)

include(00-Common)
include(LLCommon)
include(LLInventory)
include(LLMath)
include(LLMessage)
include(LLVFS)
include(LLXML)
include(LScript)
include(Linking)
include(Tut)
include(LLAddBuildTest)

include(GoogleMock)

include_directories(
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLDATABASE_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
    ${LLMESSAGE_INCLUDE_DIRS}
    ${LLINVENTORY_INCLUDE_DIRS}
    ${LLVFS_INCLUDE_DIRS}
    ${LLXML_INCLUDE_DIRS}
    ${LSCRIPT_INCLUDE_DIRS}
    ${GOOGLEMOCK_INCLUDE_DIRS}
    ${TUT_INCLUDE_DIR}
    )

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
    llscriptresource_tut.cpp
    llsdmessagebuilder_tut.cpp
    llsdmessagereader_tut.cpp
    llsd_new_tut.cpp
    llsdutil_tut.cpp
    llservicebuilder_tut.cpp
    llstreamtools_tut.cpp
    lltemplatemessagebuilder_tut.cpp
    lltut.cpp
    lluuidhashmap_tut.cpp
    message_tut.cpp
    test.cpp
    )

set(test_HEADER_FILES
    CMakeLists.txt

    debug.h
    llpipeutil.h
    llsdtraits.h
    lltut.h
    )

if (NOT WINDOWS)
  list(APPEND test_SOURCE_FILES
       llmessagetemplateparser_tut.cpp
       )
endif (NOT WINDOWS)

set_source_files_properties(${test_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND test_SOURCE_FILES ${test_HEADER_FILES})

add_executable(test ${test_SOURCE_FILES})

target_link_libraries(test
    ${LLDATABASE_LIBRARIES}
    ${LLINVENTORY_LIBRARIES}
    ${LLMESSAGE_LIBRARIES}
    ${LLMATH_LIBRARIES}
    ${LLVFS_LIBRARIES}
    ${LLXML_LIBRARIES}
    ${LSCRIPT_LIBRARIES}
    ${LLCOMMON_LIBRARIES}
    ${EXPAT_LIBRARIES}
    ${GOOGLEMOCK_LIBRARIES}
    ${PTHREAD_LIBRARY}
    ${WINDOWS_LIBRARIES}
    ${BOOST_PROGRAM_OPTIONS_LIBRARY}
    ${BOOST_REGEX_LIBRARY}
    ${DL_LIBRARY}
    ${GOOGLE_PERFTOOLS_LIBRARIES}
    )

if (WINDOWS)
  set_target_properties(test
          PROPERTIES 
          LINK_FLAGS "/NODEFAULTLIB:LIBCMT"
          LINK_FLAGS_DEBUG "/NODEFAULTLIB:\"LIBCMT;LIBCMTD;MSVCRT\""
          )
endif (WINDOWS)

get_target_property(TEST_EXE test LOCATION)

SET_TEST_PATH(DYLD_LIBRARY_PATH)

LL_TEST_COMMAND(command 
  "${DYLD_LIBRARY_PATH}"
  "${TEST_EXE}"
  "--output=${CMAKE_CURRENT_BINARY_DIR}/cpp_test_results.txt" 
  "--touch=${CMAKE_CURRENT_BINARY_DIR}/cpp_tests_ok.txt")

ADD_CUSTOM_COMMAND(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/cpp_tests_ok.txt
  COMMAND ${command}
  DEPENDS test
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
