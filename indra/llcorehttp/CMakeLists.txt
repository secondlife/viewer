# -*- cmake -*-

project(llcorehttp)

include(00-Common)
include(LLCoreHttp)
include(GoogleMock)
include(LLAddBuildTest)
include(LLCommon)
include(Tut)

include_directories (${CMAKE_CURRENT_SOURCE_DIR})

include_directories(
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLCOREHTTP_INCLUDE_DIRS}
    )

set(llcorehttp_SOURCE_FILES
    bufferarray.cpp
    httpcommon.cpp
    httprequest.cpp
    httpresponse.cpp
    httpoptions.cpp
    httpheaders.cpp
    _httprequestqueue.cpp
    _httpoperation.cpp
    _httpoprequest.cpp
    _httpreplyqueue.cpp
    _httppolicy.cpp
    _httplibcurl.cpp
    _httpservice.cpp
    _refcounted.cpp
    )

set(llcorehttp_HEADER_FILES
    CMakeLists.txt

    bufferarray.h
    httpcommon.h
    httphandler.h
    httpheaders.h
    httpoptions.h
    httprequest.h
    httpresponse.h
    _httpoperation.h
    _httpoprequest.h
    _httprequestqueue.h
    _httpreplyqueue.h
    _httpservice.h
    _httppolicy.h
    _httplibcurl.h
    _assert.h
    _refcounted.h
    _mutex.h
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
      )

  set_source_files_properties(${llcorehttp_TEST_HEADER_FILES}
                              PROPERTIES HEADER_FILE_ONLY TRUE)

  list(APPEND llcorehttp_TEST_SOURCE_FILES ${llcorehttp_TEST_HEADER_FILES})

  # LL_ADD_PROJECT_UNIT_TESTS(llcorehttp "${llcorehttp_TEST_SOURCE_FILES}")

  #    set(TEST_DEBUG on)
  set(test_libs
      ${LLCOREHTTP_LIBRARIES}
      ${WINDOWS_LIBRARIES}
      ${LLCOMMON_LIBRARIES}
      ${GOOGLEMOCK_LIBRARIES}
      ${CURL_LIBRARIES}
      ${OPENSSL_LIBRARIES}
      ${CRYPTO_LIBRARIES}
      )

  LL_ADD_INTEGRATION_TEST(llcorehttp
                          "${llcorehttp_TEST_SOURCE_FILES}"
                          "${test_libs}"
                          )

endif (LL_TESTS)

