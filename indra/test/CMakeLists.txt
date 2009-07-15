# -*- cmake -*-

project (test)

include(00-Common)
include(LLCommon)
include(LLDatabase)
include(LLInventory)
include(LLMath)
include(LLMessage)
include(LLVFS)
include(LLXML)
include(LScript)
include(Linking)
include(Tut)
include(Boost)

include_directories(
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLDATABASE_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
    ${LLMESSAGE_INCLUDE_DIRS}
    ${LLINVENTORY_INCLUDE_DIRS}
    ${LLVFS_INCLUDE_DIRS}
    ${LLXML_INCLUDE_DIRS}
    ${LSCRIPT_INCLUDE_DIRS}
    )

set(test_SOURCE_FILES
    common.cpp
    inventory.cpp
    io.cpp
#    llapp_tut.cpp						# Temporarily removed until thread issues can be solved
    llbase64_tut.cpp
    llbbox_tut.cpp
    llbboxlocal_tut.cpp
    llblowfish_tut.cpp
    llbuffer_tut.cpp
    lldate_tut.cpp
    lldependencies_tut.cpp
    lldoubledispatch_tut.cpp
    llerror_tut.cpp
    llevents_tut.cpp
    llhost_tut.cpp
    llhttpdate_tut.cpp
    llhttpclient_tut.cpp
    llhttpnode_tut.cpp
    llinventoryparcel_tut.cpp
    lliohttpserver_tut.cpp
    lljoint_tut.cpp
    llmime_tut.cpp
    llmessageconfig_tut.cpp
    llmodularmath_tut.cpp
    llnamevalue_tut.cpp
    llpermissions_tut.cpp
    llpipeutil.cpp
    llquaternion_tut.cpp
    llrandom_tut.cpp
    llrect_tut.cpp
    llsaleinfo_tut.cpp
    llscriptresource_tut.cpp
    llsdmessagebuilder_tut.cpp
    llsdmessagereader_tut.cpp
    llsd_new_tut.cpp
    llsdserialize_tut.cpp
    llsdutil_tut.cpp
    llservicebuilder_tut.cpp
    llstreamtools_tut.cpp
    llstring_tut.cpp
    lltemplatemessagebuilder_tut.cpp
    lltimestampcache_tut.cpp
    lltiming_tut.cpp
    lltranscode_tut.cpp
    lltreeiterators_tut.cpp
    lltut.cpp
    lluri_tut.cpp
    lluuidhashmap_tut.cpp
    llxfer_tut.cpp
    math.cpp
    message_tut.cpp
    reflection_tut.cpp
    stringize_tut.cpp
    test.cpp
    v2math_tut.cpp
    v3color_tut.cpp
    v3dmath_tut.cpp
    v3math_tut.cpp
    v4color_tut.cpp
    v4coloru_tut.cpp
    v4math_tut.cpp
    )

set(test_HEADER_FILES
    CMakeLists.txt

    llpipeutil.h
    llsdtraits.h
    lltut.h
    )

if (NOT WINDOWS)
  list(APPEND test_SOURCE_FILES
       llmessagetemplateparser_tut.cpp
       )
endif (NOT WINDOWS)

if (NOT DARWIN)
  list(APPEND test_SOURCE_FILES
    lldatabase_tut.cpp
    )
endif (NOT DARWIN)

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
    ${APRICONV_LIBRARIES}
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

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/cpp_tests_ok.txt
  COMMAND ${TEST_EXE}
  ARGS
    --output=${CMAKE_CURRENT_BINARY_DIR}/cpp_test_results.txt
    --touch=${CMAKE_CURRENT_BINARY_DIR}/cpp_tests_ok.txt
  DEPENDS test
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "C++ unit tests"
  )

set(test_results ${CMAKE_CURRENT_BINARY_DIR}/cpp_tests_ok.txt)

if (EXISTS /etc/debian_version_FAIL)
  # The Python tests have all kinds of wacky non-portable assumptions
  # built in.

  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/py_tests_ok.txt
    COMMAND ${PYTHON_EXECUTABLE}
    ARGS
      ${CMAKE_CURRENT_SOURCE_DIR}/test.py
      --mode=static
      --output=${CMAKE_CURRENT_BINARY_DIR}/py_test_results.txt
      --touch=${CMAKE_CURRENT_BINARY_DIR}/py_tests_ok.txt
      --mode=static
    DEPENDS test.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Python unit tests"
    )

  list(APPEND test_results ${CMAKE_CURRENT_BINARY_DIR}/py_tests_ok.txt)
endif (EXISTS /etc/debian_version_FAIL)
    
add_custom_target(tests_ok ALL DEPENDS ${test_results})
