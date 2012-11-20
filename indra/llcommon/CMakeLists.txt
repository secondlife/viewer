
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
    )

# add_executable(lltreeiterators lltreeiterators.cpp)
# 
# target_link_libraries(lltreeiterators
#     ${LLCOMMON_LIBRARIES})

set(llcommon_SOURCE_FILES
    imageids.cpp
    indra_constants.cpp
    llallocator.cpp
    llallocator_heap_profile.cpp
    llapp.cpp
    llapr.cpp
    llassettype.cpp
    llavatarname.cpp
    llbase32.cpp
    llbase64.cpp
    llcommon.cpp
    llcommonutils.cpp
    llcoros.cpp
    llcrc.cpp
    llcriticaldamp.cpp
    llcursortypes.cpp
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
    llfasttimer_class.cpp
    llfile.cpp
    llfindlocale.cpp
    llfixedbuffer.cpp
    llfoldertype.cpp
    llformat.cpp
    llframetimer.cpp
    llheartbeat.cpp
    llinitparam.cpp
    llinstancetracker.cpp
    llleap.cpp
    llleaplistener.cpp
    llliveappconfig.cpp
    lllivefile.cpp
    lllog.cpp
    llmd5.cpp
    llmemory.cpp
    llmemorystream.cpp
    llmemtype.cpp
    llmetrics.cpp
    llmetricperformancetester.cpp
    llmortician.cpp
    lloptioninterface.cpp
    llptrto.cpp 
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
    llsecondlifeurls.cpp
    llsingleton.cpp
    llstat.cpp
    llstacktrace.cpp
    llstreamqueue.cpp
    llstreamtools.cpp
    llstring.cpp
    llstringtable.cpp
    llsys.cpp
    llthread.cpp
    llthreadsafequeue.cpp
    lltimer.cpp
    lluri.cpp
    lluuid.cpp
    llworkerthread.cpp
    metaclass.cpp
    metaproperty.cpp
    reflective.cpp
    timing.cpp
    u64.cpp
    )
    
set(llcommon_HEADER_FILES
    CMakeLists.txt

    bitpack.h
    ctype_workaround.h
    doublelinkedlist.h
    fix_macros.h
    imageids.h
    indra_constants.h
    linden_common.h
    linked_lists.h
    llaccountingcost.h
    llallocator.h
    llallocator_heap_profile.h
    llagentconstants.h
    llavatarname.h
    llapp.h
    llapr.h
    llassettype.h
    llassoclist.h
    llavatarconstants.h
    llbase32.h
    llbase64.h
    llboost.h
    llchat.h
    llclickaction.h
    llcommon.h
    llcommonutils.h
    llcoros.h
    llcrc.h
    llcriticaldamp.h
    llcursortypes.h
    lldarray.h
    lldarrayptr.h
    lldate.h
    lldefs.h
    lldependencies.h
    lldeleteutils.h
    lldepthstack.h
    lldictionary.h
    lldlinked.h
    lldoubledispatch.h
    lldqueueptr.h
    llendianswizzle.h
    llenum.h
    llerror.h
    llerrorcontrol.h
    llerrorlegacy.h
    llerrorthread.h
    llevent.h
    lleventapi.h
    lleventcoro.h
    lleventdispatcher.h
    lleventfilter.h
    llevents.h
    lleventemitter.h
    llextendedstatus.h
    llfasttimer.h
    llfasttimer_class.h
    llfile.h
    llfindlocale.h
    llfixedbuffer.h
    llfoldertype.h
    llformat.h
    llframetimer.h
    llhandle.h
    llhash.h
    llheartbeat.h
    llhttpstatuscodes.h
    llindexedqueue.h
    llinitparam.h
    llinstancetracker.h
    llkeythrottle.h
    lllazy.h
    llleap.h
    llleaplistener.h
    lllistenerwrapper.h
    lllinkedqueue.h
    llliveappconfig.h
    lllivefile.h
    lllocalidhashmap.h
    lllog.h
    lllslconstants.h
    llmap.h
    llmd5.h
    llmemory.h
    llmemorystream.h
    llmemtype.h
    llmetrics.h
    llmetricperformancetester.h
    llmortician.h
    llnametable.h
    lloptioninterface.h
    llpointer.h
    llpreprocessor.h
    llpriqueuemap.h
    llprocess.h
    llprocessor.h
    llptrskiplist.h
    llptrskipmap.h
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
    llsecondlifeurls.h
    llsimplehash.h
    llsingleton.h
    llskiplist.h
    llskipmap.h
    llsortedvector.h
    llstack.h
    llstacktrace.h
    llstat.h
    llstatenums.h
    llstl.h
    llstreamqueue.h
    llstreamtools.h
    llstrider.h
    llstring.h
    llstringtable.h
    llsys.h
    llthread.h
    llthreadsafequeue.h
    lltimer.h
    lltreeiterators.h
    lltypeinfolookup.h
    lluri.h
    lluuid.h
    lluuidhashmap.h
    llversionserver.h
    llversionviewer.h
    llworkerthread.h
    ll_template_cast.h
    metaclass.h
    metaclasst.h
    metaproperty.h
    metapropertyt.h
    reflective.h
    reflectivet.h
    roles_constants.h
    stdenums.h
    stdtypes.h
    string_table.h
    stringize.h
    timer.h
    timing.h
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
  LL_ADD_INTEGRATION_TEST(lllazy "" "${test_libs}")
  LL_ADD_INTEGRATION_TEST(llprocessor "" "${test_libs}")
  LL_ADD_INTEGRATION_TEST(llrand "" "${test_libs}")
  LL_ADD_INTEGRATION_TEST(llsdserialize "" "${test_libs}")
  LL_ADD_INTEGRATION_TEST(llsingleton "" "${test_libs}")                          
  LL_ADD_INTEGRATION_TEST(llstring "" "${test_libs}")
  LL_ADD_INTEGRATION_TEST(lltreeiterators "" "${test_libs}")
  LL_ADD_INTEGRATION_TEST(lluri "" "${test_libs}")
  LL_ADD_INTEGRATION_TEST(reflection "" "${test_libs}")
  LL_ADD_INTEGRATION_TEST(stringize "" "${test_libs}")
  LL_ADD_INTEGRATION_TEST(lleventdispatcher "" "${test_libs}")
  LL_ADD_INTEGRATION_TEST(llprocess "" "${test_libs}")
  LL_ADD_INTEGRATION_TEST(llleap "" "${test_libs}")
  LL_ADD_INTEGRATION_TEST(llstreamqueue "" "${test_libs}")

  # *TODO - reenable these once tcmalloc libs no longer break the build.
  #ADD_BUILD_TEST(llallocator llcommon)
  #ADD_BUILD_TEST(llallocator_heap_profile llcommon)
  #ADD_BUILD_TEST(llmemtype llcommon)
endif (LL_TESTS)
