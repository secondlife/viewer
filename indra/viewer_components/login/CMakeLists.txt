# -*- cmake -*-

project(login)

include(00-Common)
if(LL_TESTS)
  include(LLAddBuildTest)
  include(Doctest)
endif(LL_TESTS)
include(LLCommon)
include(LLCoreHttp)

set(login_SOURCE_FILES
    lllogin.cpp
    )

set(login_HEADER_FILES
    lllogin.h
    )

list(APPEND 
    login_SOURCE_FILES 
    ${login_HEADER_FILES} 
    )

add_library(lllogin 
            ${login_SOURCE_FILES}
            )
target_include_directories( lllogin  INTERFACE   ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(lllogin
        llmessage
        llcorehttp
        llcommon
        llmath
        llxml
    )

if(LL_TESTS)
  enable_testing()
  SET(lllogin_TEST_SOURCE_FILES
      lllogin.cpp
      )
  set_source_files_properties(
    lllogin.cpp
    PROPERTIES
    LL_TEST_ADDITIONAL_LIBRARIES llmessage llcorehttp llcommon
    )

  LL_ADD_PROJECT_UNIT_TESTS(lllogin "${lllogin_TEST_SOURCE_FILES}")

  add_executable(login_doctest
      tests/lllogin_doctest.cpp
      ${CMAKE_SOURCE_DIR}/test/doctest_main.cpp
  )

  target_include_directories(login_doctest
      PRIVATE
          ${DOCTEST_INCLUDE_DIR}
          ${CMAKE_SOURCE_DIR}/test
          ${CMAKE_CURRENT_SOURCE_DIR}
          ${CMAKE_CURRENT_SOURCE_DIR}/tests
  )

  target_link_libraries(login_doctest
      lllogin
      llmessage
      llcorehttp
      llcommon
      llmath
      llxml
  )

  add_test(NAME login_doctest COMMAND login_doctest)
endif(LL_TESTS)
