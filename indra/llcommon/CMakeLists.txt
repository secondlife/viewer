
# -*- cmake -*-

project(llcommon)

include(00-Common)
include(LLCommon)
include(Linking)
include(Boost)
include(LLSharedLibs)
include(GoogleBreakpad)
include(GooglePerfTools)
include(Copy3rdPartyLibs)
include(ZLIB)

include_directories(
    ${EXPAT_INCLUDE_DIRS}
    ${LLCOMMON_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIRS}
    ${BREAKPAD_INCLUDE_DIRECTORIES}
    )

# add_executable(lltreeiterators lltreeiterators.cpp)
# 
# target_link_libraries(lltreeiterators
#     ${LLCOMMON_LIBRARIES})

set(llcommon_SOURCE_FILES
    indra_constants.cpp
    llallocator.cpp
    llallocator_heap_profile.cpp
    llapp.cpp
    llapr.cpp
    llassettype.cpp
    llbase32.cpp
    llbase64.cpp
	llbitpack.cpp
    llcommon.cpp
    llcommonutils.cpp
    llcoros.cpp
    llcrc.cpp
    llcriticaldamp.cpp
    lldate.cpp
    lldependencies.cpp
    lldictionary.cpp
    llerror.cpp
    llerrorthread.cpp
    llevent.cpp
    lleventapi.cpp
    lleventcoro.cpp
    lleventdispatcher.cpp
    lleventfilter.cpp
    llevents.cpp
    lleventtimer.cpp
    llfasttimer.cpp
    llfile.cpp
    llfindlocale.cpp
    llfixedbuffer.cpp
    llformat.cpp
    llframetimer.cpp
    llheartbeat.cpp
    llinitparam.cpp
    llinstancetracker.cpp
    llleap.cpp
    llleaplistener.cpp
    llliveappconfig.cpp
    lllivefile.cpp
    llmd5.cpp
    llmemory.cpp
    llmemorystream.cpp
    llmetrics.cpp
    llmetricperformancetester.cpp
    llmortician.cpp
    llmutex.cpp
    llptrto.cpp 
    llpredicate.cpp
    llprocess.cpp
    llprocessor.cpp
    llqueuedthread.cpp
    llrand.cpp
    llrefcount.cpp
    llrun.cpp
    llsd.cpp
    llsdparam.cpp
    llsdserialize.cpp
    llsdserialize_xml.cpp
    llsdutil.cpp
    llsingleton.cpp
    llstacktrace.cpp
    llstreamqueue.cpp
    llstreamtools.cpp
    llstring.cpp
    llstringtable.cpp
    llsys.cpp
    llthread.cpp
    llthreadlocalstorage.cpp
    llthreadsafequeue.cpp
    lltimer.cpp
    lltrace.cpp
    lltraceaccumulators.cpp
    lltracerecording.cpp
    lltracethreadrecorder.cpp
    lluri.cpp
    lluuid.cpp
    llworkerthread.cpp
    timing.cpp
    u64.cpp
    )
    
set(llcommon_HEADER_FILES
    CMakeLists.txt

    ctype_workaround.h
    fix_macros.h
    indra_constants.h
    linden_common.h
    llallocator.h
    llallocator_heap_profile.h
    llapp.h
    llapr.h
    llassettype.h
    llbase32.h
    llbase64.h
    llbitpack.h
    llboost.h
    llcommon.h
    llcommonutils.h
    llcoros.h
    llcrc.h
    llcriticaldamp.h
    lldate.h
    lldefs.h
    lldependencies.h
    lldepthstack.h
    lldictionary.h
    lldoubledispatch.h
    llendianswizzle.h
    llerror.h
    llerrorcontrol.h
    llerrorthread.h
    llevent.h
    lleventapi.h
    lleventcoro.h
    lleventdispatcher.h
    lleventfilter.h
    llevents.h
    lleventemitter.h
    llfasttimer.h
    llfile.h
    llfindlocale.h
    llfixedbuffer.h
    llformat.h
    llframetimer.h
    llhandle.h
    llhash.h
    llheartbeat.h
    llhttpstatuscodes.h
    llindexedvector.h
    llinitparam.h
    llinstancetracker.h
    llkeythrottle.h
    llleap.h
    llleaplistener.h
    lllistenerwrapper.h
    llliveappconfig.h
    lllivefile.h
    llmd5.h
    llmemory.h
    llmemorystream.h
    llmetrics.h
    llmetricperformancetester.h
    llmortician.h
    llmutex.h
    llpointer.h
    llpredicate.h
    llpreprocessor.h
    llpriqueuemap.h
    llprocess.h
    llprocessor.h
    llptrto.h
    llqueuedthread.h
    llrand.h
    llrefcount.h
    llregistry.h
    llrun.h
    llrefcount.h
    llsafehandle.h
    llsd.h
    llsdparam.h
    llsdserialize.h
    llsdserialize_xml.h
    llsdutil.h
    llsimplehash.h
    llsingleton.h
    llstacktrace.h
    llstl.h
    llstreamqueue.h
    llstreamtools.h
    llstrider.h
    llstring.h
    llstringtable.h
    llsys.h
    llthread.h
    llthreadlocalstorage.h
    llthreadsafequeue.h
    lltimer.h
    lltrace.h
	lltraceaccumulators.h
    lltracerecording.h
    lltracethreadrecorder.h
    lltreeiterators.h
    llunit.h
    lluri.h
    lluuid.h
	llwin32headers.h
    llwin32headerslean.h
    llworkerthread.h
    stdtypes.h
    stringize.h
    timer.h
    u64.h
    )

set_source_files_properties(${llcommon_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND llcommon_SOURCE_FILES ${llcommon_HEADER_FILES})

if(LLCOMMON_LINK_SHARED)
  add_library (llcommon SHARED ${llcommon_SOURCE_FILES})
  if(NOT WORD_SIZE EQUAL 32)
    if(WINDOWS)
      add_definitions(/FIXED:NO)
    else(WINDOWS) # not windows therefore gcc LINUX and DARWIN
      add_definitions(-fPIC)
    endif(WINDOWS)
  endif(NOT WORD_SIZE EQUAL 32)
  if(WINDOWS)
    # always generate llcommon.pdb, even for "Release" builds
    set_target_properties(llcommon PROPERTIES LINK_FLAGS "/DEBUG")
  endif(WINDOWS)
  ll_stage_sharedlib(llcommon)
else(LLCOMMON_LINK_SHARED)
    add_library (llcommon ${llcommon_SOURCE_FILES})
endif(LLCOMMON_LINK_SHARED)

target_link_libraries(
    llcommon
    ${BREAKPAD_EXCEPTION_HANDLER_LIBRARIES}
    ${APRUTIL_LIBRARIES}
    ${APR_LIBRARIES}
    ${EXPAT_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${WINDOWS_LIBRARIES}
    ${BOOST_PROGRAM_OPTIONS_LIBRARY}
    ${BOOST_REGEX_LIBRARY}
    ${GOOGLE_PERFTOOLS_LIBRARIES}
    )

if (DARWIN)
  include(CMakeFindFrameworks)
  find_library(CARBON_LIBRARY Carbon)
  target_link_libraries(llcommon ${CARBON_LIBRARY})
endif (DARWIN)

add_dependencies(llcommon stage_third_party_libs)

if (LL_TESTS)
  include(LLAddBuildTest)
  SET(llcommon_TEST_SOURCE_FILES
    # unit-testing llcommon is not possible right now as the test-harness *itself* depends upon llcommon, causing a circular dependency.  Add your 'unit' tests as integration tests for now.
    )
  LL_ADD_PROJECT_UNIT_TESTS(llcommon "${llcommon_TEST_SOURCE_FILES}")

  #set(TEST_DEBUG on)
  set(test_libs llcommon ${LLCOMMON_LIBRARIES} ${WINDOWS_LIBRARIES} ${GOOGLEMOCK_LIBRARIES})
  LL_ADD_INTEGRATION_TEST(commonmisc "" "${test_libs}")
  LL_ADD_INTEGRATION_TEST(bitpack "" "${test_libs}")
  LL_ADD_INTEGRATION_TEST(llbase64 "" "${test_libs}")
  LL_ADD_INTEGRATION_TEST(lldate "" "${test_libs}")
  LL_ADD_INTEGRATION_TEST(lldependencies "" "${test_libs}")
  LL_ADD_INTEGRATION_TEST(llerror "" "${test_libs}")
  LL_ADD_INTEGRATION_TEST(llframetimer "" "${test_libs}")
  LL_ADD_INTEGRATION_TEST(llinstancetracker "" "${test_libs}")
  LL_ADD_INTEGRATION_TEST(llprocessor "" "${test_libs}")
  LL_ADD_INTEGRATION_TEST(llrand "" "${test_libs}")
  LL_ADD_INTEGRATION_TEST(llsdserialize "" "${test_libs}")
  LL_ADD_INTEGRATION_TEST(llsingleton "" "${test_libs}")                          
  LL_ADD_INTEGRATION_TEST(llstring "" "${test_libs}")
  LL_ADD_INTEGRATION_TEST(lltreeiterators "" "${test_libs}")
  LL_ADD_INTEGRATION_TEST(lluri "" "${test_libs}")
  LL_ADD_INTEGRATION_TEST(llunits "" "${test_libs}")
  LL_ADD_INTEGRATION_TEST(stringize "" "${test_libs}")
  LL_ADD_INTEGRATION_TEST(lleventdispatcher "" "${test_libs}")
  LL_ADD_INTEGRATION_TEST(lleventcoro "" "${test_libs};${BOOST_CONTEXT_LIBRARY}")
  LL_ADD_INTEGRATION_TEST(llprocess "" "${test_libs}")
  LL_ADD_INTEGRATION_TEST(llleap "" "${test_libs}")
  LL_ADD_INTEGRATION_TEST(llstreamqueue "" "${test_libs}")

  # *TODO - reenable these once tcmalloc libs no longer break the build.
  #ADD_BUILD_TEST(llallocator llcommon)
  #ADD_BUILD_TEST(llallocator_heap_profile llcommon)
  #ADD_BUILD_TEST(llmemtype llcommon)
endif (LL_TESTS)
