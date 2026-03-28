# -*- cmake -*-

project(llcorehttp)

include(00-Common)
include(CURL)
include(OpenSSL)
include(NGHTTP2)
include(ZLIBNG)
include(LLCoreHttp)
include(LLAddBuildTest)
include(LLCommon)
include(Tut)
include(bugsplat)

set(llcorehttp_SOURCE_FILES
    bufferarray.cpp
    bufferstream.cpp
    httpcommon.cpp
    llhttpconstants.cpp
    httpheaders.cpp
    httpoptions.cpp
    httprequest.cpp
    httpresponse.cpp
    httpstats.cpp
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
    llhttpconstants.h
    httphandler.h
    httpheaders.h
    httpoptions.h
    httprequest.h
    httpresponse.h
    httpstats.h
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

if (DARWIN OR LINUX)
  # Boost headers define unused members in condition_variable so...
  set_source_files_properties(${llcorehttp_SOURCE_FILES}
                              PROPERTIES COMPILE_FLAGS -Wno-unused-variable)
endif (DARWIN OR LINUX)

list(APPEND llcorehttp_SOURCE_FILES ${llcorehttp_HEADER_FILES})

add_library (llcorehttp ${llcorehttp_SOURCE_FILES})
target_link_libraries(
        llcorehttp
        llcommon
        ll::libcurl
        ll::openssl
        ll::nghttp2
  )
target_include_directories( llcorehttp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
# llmessage depends on llcorehttp, yet llcorehttp also depends on llmessage (at least for includes).
# Cannot/Should not use target_link_libraries here to add llmessage to the dependencies, as that would
# lead to circular dependencies (or in case of cmake, the first project declaring it's dependencies wins)
target_include_directories( llcorehttp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../llmessage)

# tests
set(LLCOREHTTP_TESTS ON CACHE BOOL
    "Build and run llcorehttp integration tests specifically")
if (LL_TESTS AND LLCOREHTTP_TESTS)
  #
  # Example Programs
  #
  SET(llcorehttp_EXAMPLE_SOURCE_FILES
      examples/http_texture_load.cpp
      )

  set(example_libs
          llcorehttp
          llmessage
          llcommon
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
                          LINK_FLAGS "/debug /NODEFAULTLIB:LIBCMT /SUBSYSTEM:CONSOLE"
                          LINK_FLAGS_DEBUG "/NODEFAULTLIB:\"LIBCMT;LIBCMTD;MSVCRT\" /INCREMENTAL:NO"
                          LINK_FLAGS_RELEASE ""
                          )
  endif (WINDOWS)

  target_link_libraries(http_texture_load ${example_libs})

  add_subdirectory(tests_doctest)

endif (LL_TESTS AND LLCOREHTTP_TESTS)
