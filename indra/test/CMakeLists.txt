# -*- cmake -*-

project (lltest)

include(00-Common)
include(LLCommon)
include(LLCoreHttp)
include(Linking)
include(Tut)
include(LLAddBuildTest)
include(bugsplat)

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

list(APPEND test_SOURCE_FILES ${test_HEADER_FILES})

add_executable(lltest ${test_SOURCE_FILES})

target_link_libraries(lltest
        llinventory
        llmessage
        llmath
        llfilesystem
        llxml
        llcommon
        llcorehttp
        )

if (WINDOWS)
  set_target_properties(lltest
          PROPERTIES 
          LINK_FLAGS "/NODEFAULTLIB:LIBCMT"
          LINK_FLAGS_DEBUG "/NODEFAULTLIB:\"LIBCMT;LIBCMTD;MSVCRT\""
          RUNTIME_OUTPUT_DIRECTORY "${EXE_STAGING_DIR}"
          )
elseif (DARWIN)
  # Support our "@executable_path/../Resources" load path for our test
  # executable. This SHOULD properly be "$<TARGET_FILE_DIR:lltest>/Resources",
  # but the CMake $<TARGET_FILE_DIR> generator expression isn't evaluated by
  # CREATE_LINK, so fudge it.
  file(CREATE_LINK "../sharedlibs/Release/Resources" "${CMAKE_BINARY_DIR}/test/Resources"
       SYMBOLIC)
endif (WINDOWS)

set(TEST_EXE $<TARGET_FILE:lltest>)

SET_TEST_PATH(LD_LIBRARY_PATH)

LL_TEST_COMMAND(command 
  "${LD_LIBRARY_PATH}"
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
    if(DARWIN)
      # Support our "@executable_path/../Resources" load path for our test
      # executable. This SHOULD properly be "$<TARGET_FILE_DIR:lltest>/Resources",
      # but the CMake $<TARGET_FILE_DIR> generator expression isn't evaluated by
      # CREATE_LINK, so fudge it.
      add_custom_command( TARGET lltest POST_BUILD
              COMMAND cmake -E create_symlink ${SHARED_LIB_STAGING_DIR} ${CMAKE_BINARY_DIR}/test/Resources
              )
    endif()
endif (LL_TESTS)
