# -*- cmake -*-

project(llplugin)

include(00-Common)
include(CURL)
include(LLCommon)
include(LLImage)
include(LLMath)
include(LLMessage)
include(LLRender)
include(LLXML)
include(LLWindow)

include_directories(
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLIMAGE_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
    ${LLMESSAGE_INCLUDE_DIRS}
    ${LLRENDER_INCLUDE_DIRS}
    ${LLXML_INCLUDE_DIRS}
    ${LLWINDOW_INCLUDE_DIRS}
    ${LLQTWEBKIT_INCLUDE_DIR}
    )
include_directories(SYSTEM
    ${LLCOMMON_SYSTEM_INCLUDE_DIRS}
    ${LLXML_SYSTEM_INCLUDE_DIRS}
    )

set(llplugin_SOURCE_FILES
    llpluginclassmedia.cpp
    llplugincookiestore.cpp
    llplugininstance.cpp
    llpluginmessage.cpp
    llpluginmessagepipe.cpp
    llpluginprocesschild.cpp
    llpluginprocessparent.cpp
    llpluginsharedmemory.cpp
    )

set(llplugin_HEADER_FILES
    CMakeLists.txt

    llpluginclassmedia.h
    llpluginclassmediaowner.h
    llplugincookiestore.h
    llplugininstance.h
    llpluginmessage.h
    llpluginmessageclasses.h
    llpluginmessagepipe.h
    llpluginprocesschild.h
    llpluginprocessparent.h
    llpluginsharedmemory.h
    )

set_source_files_properties(${llplugin_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

if(NOT WORD_SIZE EQUAL 32)
  if(WINDOWS)
    add_definitions(/FIXED:NO)
  else(WINDOWS) # not windows therefore gcc LINUX and DARWIN
    add_definitions(-fPIC)
  endif(WINDOWS)
endif(NOT WORD_SIZE EQUAL 32)

list(APPEND llplugin_SOURCE_FILES ${llplugin_HEADER_FILES})

add_library (llplugin ${llplugin_SOURCE_FILES})

add_subdirectory(slplugin)

# Add tests
if (LL_TESTS)
    include(LLAddBuildTest)
    # UNIT TESTS
    SET(llplugin_TEST_SOURCE_FILES
      llplugincookiestore.cpp
      )

    # llplugincookiestore has a dependency on curl, so we need to link the curl library into the test.
    set_source_files_properties(
      llplugincookiestore.cpp
      PROPERTIES
        LL_TEST_ADDITIONAL_LIBRARIES "${CURL_LIBRARIES}"
      )

    LL_ADD_PROJECT_UNIT_TESTS(llplugin "${llplugin_TEST_SOURCE_FILES}")
endif (LL_TESTS)
