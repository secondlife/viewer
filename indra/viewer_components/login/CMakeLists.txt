# -*- cmake -*-

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

add_library(lllogin STATIC
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

if(BUILD_TESTING)
  SET(lllogin_TEST_SOURCE_FILES
      lllogin.cpp
      )
  set_source_files_properties(
    lllogin.cpp
    PROPERTIES
    LL_TEST_ADDITIONAL_LIBRARIES llmessage llcorehttp llcommon
    )

  LL_ADD_PROJECT_UNIT_TESTS(lllogin "${lllogin_TEST_SOURCE_FILES}")
endif()
