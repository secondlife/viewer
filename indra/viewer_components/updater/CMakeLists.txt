# -*- cmake -*-

project(updater_service)

include(00-Common)
if(LL_TESTS)
  include(LLAddBuildTest)
endif(LL_TESTS)
include(Boost)
include(CMakeCopyIfDifferent)
include(CURL)
include(LLCommon)
include(LLCoreHttp)
include(LLMessage)
include(LLPlugin)
include(LLVFS)

include_directories(
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLCOREHTTP_INCLUDE_DIRS}
    ${LLMESSAGE_INCLUDE_DIRS}
    ${LLPLUGIN_INCLUDE_DIRS}
    ${LLVFS_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/newview
    )
include_directories(SYSTEM
    ${LLCOMMON_SYSTEM_INCLUDE_DIRS}
    )

set(updater_service_SOURCE_FILES
    llupdaterservice.cpp
    llupdatechecker.cpp
    llupdatedownloader.cpp
    llupdateinstaller.cpp
    )

set(updater_service_HEADER_FILES
    llupdaterservice.h
    llupdatechecker.h
    llupdatedownloader.h
    llupdateinstaller.h
    )

set_source_files_properties(${updater_service_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

set_source_files_properties(
   llupdaterservice.cpp 
   PROPERTIES
   COMPILE_DEFINITIONS "${VIEWER_CHANNEL_VERSION_DEFINES}" # see BuildVersion.cmake
   )

list(APPEND 
    updater_service_SOURCE_FILES 
    ${updater_service_HEADER_FILES} 
    )

add_library(llupdaterservice
            ${updater_service_SOURCE_FILES}
            )

target_link_libraries(llupdaterservice
    ${LLCOMMON_LIBRARIES}
    ${LLMESSAGE_LIBRARIES}
    ${LLCOREHTTP_LIBRARIES}
    ${LLPLUGIN_LIBRARIES}
    ${LLVFS_LIBRARIES}
    )

if(LL_TESTS)
if (NOT LINUX)
  SET(llupdater_service_TEST_SOURCE_FILES
      llupdaterservice.cpp
      )

set(test_libs
    ${LLCOMMON_LIBRARIES}
    ${BOOST_COROUTINE_LIBRARY} 
    ${BOOST_CONTEXT_LIBRARY} 
    ${BOOST_THREAD_LIBRARY} 
    ${BOOST_SYSTEM_LIBRARY})

set_source_files_properties(
    llupdaterservice.cpp
    PROPERTIES
      LL_TEST_ADDITIONAL_LIBRARIES ${test_libs}
# *NOTE:Mani - I was trying to use the preprocessor seam to mock out
#              llifstream (and other) llcommon classes. It didn't work
#              because of the windows declspec(dllimport)attribute.
#     LL_TEST_ADDITIONAL_CFLAGS "-Dllifstream=llus_mock_llifstream"
    )

  LL_ADD_PROJECT_UNIT_TESTS(llupdaterservice "${llupdater_service_TEST_SOURCE_FILES}" ${test_libs})
endif (NOT LINUX)
endif(LL_TESTS)

set(UPDATER_INCLUDE_DIRS 
  ${LIBS_OPEN_DIR}/viewer_components/updater 
  CACHE INTERNAL ""
)

set(UPDATER_LIBRARIES 
  llupdaterservice
  CACHE INTERNAL ""
)
