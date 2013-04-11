# -*- cmake -*-

project(login)

include(00-Common)
if(LL_TESTS)
  include(LLAddBuildTest)
endif(LL_TESTS)
include(LLCommon)
include(LLMath)
include(LLXML)

include_directories(
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
    ${LLXML_INCLUDE_DIRS}
    )
include_directories(SYSTEM
    ${LLCOMMON_SYSTEM_INCLUDE_DIRS}
    ${LLXML_SYSTEM_INCLUDE_DIRS}
    )

set(login_SOURCE_FILES
    lllogin.cpp
    )

set(login_HEADER_FILES
    lllogin.h
    )

set_source_files_properties(${login_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND 
    login_SOURCE_FILES 
    ${login_HEADER_FILES} 
    )

add_library(lllogin 
            ${login_SOURCE_FILES}
            )

target_link_libraries(lllogin
    ${LLCOMMON_LIBRARIES}
    ${LLMATH_LIBRARIES}
    ${LLXML_LIBRARIES}
    )

if(LL_TESTS)
  SET(lllogin_TEST_SOURCE_FILES
      lllogin.cpp
      )

  LL_ADD_PROJECT_UNIT_TESTS(lllogin "${lllogin_TEST_SOURCE_FILES}")
endif(LL_TESTS)
