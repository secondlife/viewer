# -*- cmake -*-

project(llcorehttp)

include(00-Common)
include(GoogleMock)
include(CURL)
include(CARes)
include(OpenSSL)
include(ZLIB)
include(LLCoreHttp)
include(LLAddBuildTest)
include(LLMessage)
include(LLCommon)
include(Tut)

include_directories (${CMAKE_CURRENT_SOURCE_DIR})

include_directories(
    ${LLMESSAGE_INCLUDE_DIRS}
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLCOREHTTP_INCLUDE_DIRS}
    )

set(llcorehttp_SOURCE_FILES
    bufferarray.cpp
    bufferstream.cpp
    httpcommon.cpp
    httpheaders.cpp
    httpoptions.cpp
    httprequest.cpp
    httpresponse.cpp
    _httplibcurl.cpp
    _httpopcancel.cpp
    _httpoperation.cpp
    _httpoprequest.cpp
    _httpopsetget.cpp
    _httpopsetpriority.cpp
    _httppolicy.cpp
    _httppolicyclass.cpp
    _httppolicyglobal.cpp
    _httpreplyqueue.cpp
    _httprequestqueue.cpp
    _httpservice.cpp
    _refcounted.cpp
    )

set(llcorehttp_HEADER_FILES
    CMakeLists.txt

    bufferarray.h
    bufferstream.h
    httpcommon.h
    httphandler.h
    httpheaders.h
    httpoptions.h
    httprequest.h
    httpresponse.h
    _httpinternal.h
    _httplibcurl.h
    _httpopcancel.h
    _httpoperation.h
    _httpoprequest.h
    _httpopsetget.h
    _httpopsetpriority.h
    _httppolicy.h
    _httppolicyclass.h
    _httppolicyglobal.h
    _httpreadyqueue.h
    _httpreplyqueue.h
    _httprequestqueue.h
    _httpservice.h
    _mutex.h
    _refcounted.h
    _thread.h
    )

set_source_files_properties(${llcorehttp_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)
if (DARWIN OR LINUX)
  # Boost headers define unused members in condition_variable so...
  set_source_files_properties(${llcorehttp_SOURCE_FILES}
                              PROPERTIES COMPILE_FLAGS -Wno-unused-variable)
endif (DARWIN OR LINUX)

list(APPEND llcorehttp_SOURCE_FILES ${llcorehttp_HEADER_FILES})

add_library (llcorehttp ${llcorehttp_SOURCE_FILES})
target_link_libraries(
  llcorehttp
  ${CURL_LIBRARIES}
  ${CARES_LIBRARIES}
  ${OPENSSL_LIBRARIES}
  ${CRYPTO_LIBRARIES}
  ${BOOST_THREAD_LIBRARY}
  )

# tests
if (LL_TESTS)
  SET(llcorehttp_TEST_SOURCE_FILES
      tests/test_allocator.cpp
      )

  set(llcorehttp_TEST_HEADER_FILS
      tests/test_httpstatus.hpp
      tests/test_refcounted.hpp
      tests/test_httpoperation.hpp
      tests/test_httprequest.hpp
      tests/test_httprequestqueue.hpp
      tests/test_httpheaders.hpp
      tests/test_bufferarray.hpp
      tests/test_bufferstream.hpp
      )

  set_source_files_properties(${llcorehttp_TEST_HEADER_FILES}
                              PROPERTIES HEADER_FILE_ONLY TRUE)

  list(APPEND llcorehttp_TEST_SOURCE_FILES ${llcorehttp_TEST_HEADER_FILES})

  # LL_ADD_PROJECT_UNIT_TESTS(llcorehttp "${llcorehttp_TEST_SOURCE_FILES}")

  #    set(TEST_DEBUG on)
  set(test_libs
      ${LLCOREHTTP_LIBRARIES}
      ${WINDOWS_LIBRARIES}
      ${LLMESSAGE_LIBRARIES}
      ${LLCOMMON_LIBRARIES}
      ${GOOGLEMOCK_LIBRARIES}
      ${CURL_LIBRARIES}
      ${CARES_LIBRARIES}
      ${OPENSSL_LIBRARIES}
      ${CRYPTO_LIBRARIES}
      ${BOOST_THREAD_LIBRARY}
      )

  LL_ADD_INTEGRATION_TEST(llcorehttp
                          "${llcorehttp_TEST_SOURCE_FILES}"
                          "${test_libs}"
                          ${PYTHON_EXECUTABLE}
                          "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_llcorehttp_peer.py"
                          )

  #
  # Example Programs
  #
  SET(llcorehttp_EXAMPLE_SOURCE_FILES
      examples/http_texture_load.cpp
      )

  set(example_libs
      ${LLCOREHTTP_LIBRARIES}
      ${WINDOWS_LIBRARIES}
      ${LLMESSAGE_LIBRARIES}
      ${LLCOMMON_LIBRARIES}
      ${GOOGLEMOCK_LIBRARIES}
      ${CURL_LIBRARIES}
      ${CARES_LIBRARIES}
      ${OPENSSL_LIBRARIES}
      ${CRYPTO_LIBRARIES}
      ${BOOST_THREAD_LIBRARY}
      )

  add_executable(http_texture_load
                 ${llcorehttp_EXAMPLE_SOURCE_FILES}
                 )
  set_target_properties(http_texture_load
                        PROPERTIES
                        RUNTIME_OUTPUT_DIRECTORY "${EXE_STAGING_DIR}"
                        )

  if (WINDOWS)
    # The following come from LLAddBuildTest.cmake's INTEGRATION_TEST_xxxx target.
    set_target_properties(http_texture_load
                          PROPERTIES
                          LINK_FLAGS "/debug /NODEFAULTLIB:LIBCMT /SUBSYSTEM:WINDOWS /INCLUDE:__tcmalloc"
                          LINK_FLAGS_DEBUG "/NODEFAULTLIB:\"LIBCMT;LIBCMTD;MSVCRT\" /INCREMENTAL:NO"
                          LINK_FLAGS_RELEASE ""
                          )
  endif (WINDOWS)

  target_link_libraries(http_texture_load ${example_libs})

endif (LL_TESTS)

