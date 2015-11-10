# -*- cmake -*-

project(llmessage)

include(00-Common)
include(GoogleMock)
include(LLAddBuildTest)
include(LLCommon)
include(LLCoreHttp)
include(LLMath)
include(LLMessage)
include(LLVFS)
include(LLAddBuildTest)
include(Python)
include(Tut)
include(Python)

include_directories (${CMAKE_CURRENT_SOURCE_DIR})

include_directories(
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLCOREHTTP_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
    ${LLMESSAGE_INCLUDE_DIRS}
    ${LLVFS_INCLUDE_DIRS}
    )

set(llmessage_SOURCE_FILES
    llares.cpp
    llareslistener.cpp
    llassetstorage.cpp
    llavatarname.cpp
    llavatarnamecache.cpp
    llblowfishcipher.cpp
    llbuffer.cpp
    llbufferstream.cpp
    llcachename.cpp
    llchainio.cpp
    llcircuit.cpp
    llclassifiedflags.cpp
    llcorehttputil.cpp
    llcurl.cpp
    lldatapacker.cpp
    lldispatcher.cpp
    llexperiencecache.cpp
    llfiltersd2xmlrpc.cpp
    llhost.cpp
    llhttpassetstorage.cpp
    llhttpclient.cpp
    llhttpclientadapter.cpp
    llhttpconstants.cpp
    llhttpnode.cpp
    llhttpsender.cpp
    llinstantmessage.cpp
    lliobuffer.cpp
    lliohttpserver.cpp
    lliopipe.cpp
    lliosocket.cpp
    llioutil.cpp
    llmail.cpp
    llmessagebuilder.cpp
    llmessageconfig.cpp
    llmessagereader.cpp
    llmessagetemplate.cpp
    llmessagetemplateparser.cpp
    llmessagethrottle.cpp
    llnamevalue.cpp
    llnullcipher.cpp
    llpacketack.cpp
    llpacketbuffer.cpp
    llpacketring.cpp
    llpartdata.cpp
    llproxy.cpp
    llpumpio.cpp
    llsdappservices.cpp
    llsdhttpserver.cpp
    llsdmessage.cpp
    llsdmessagebuilder.cpp
    llsdmessagereader.cpp
    llsdrpcclient.cpp
    llsdrpcserver.cpp
    llservicebuilder.cpp
    llservice.cpp
    llstoredmessage.cpp
    lltemplatemessagebuilder.cpp
    lltemplatemessagedispatcher.cpp
    lltemplatemessagereader.cpp
    llthrottle.cpp
    lltransfermanager.cpp
    lltransfersourceasset.cpp
    lltransfersourcefile.cpp
    lltransfertargetfile.cpp
    lltransfertargetvfile.cpp
    lltrustedmessageservice.cpp
    llurlrequest.cpp
    lluseroperation.cpp
    llxfer.cpp
    llxfer_file.cpp
    llxfermanager.cpp
    llxfer_mem.cpp
    llxfer_vfile.cpp
    llxorcipher.cpp
    machine.cpp
    message.cpp
    message_prehash.cpp
    message_string_table.cpp
    net.cpp
    partsyspacket.cpp
    patch_code.cpp
    patch_dct.cpp
    patch_idct.cpp
    sound_ids.cpp
    )

set(llmessage_HEADER_FILES
    CMakeLists.txt

    llares.h
    llareslistener.h
    llassetstorage.h
    llavatarname.h
    llavatarnamecache.h
    llblowfishcipher.h
    llbuffer.h
    llbufferstream.h
    llcachename.h
    llchainio.h
    llcipher.h
    llcircuit.h
    llclassifiedflags.h
    llcorehttputil.h
    llcurl.h
    lldatapacker.h
    lldbstrings.h
    lldispatcher.h
    lleventflags.h
    llexperiencecache.h
    llextendedstatus.h
    llfiltersd2xmlrpc.h
    llfollowcamparams.h
    llhost.h
    llhttpassetstorage.h
    llhttpclient.h
    llhttpclientinterface.h
    llhttpclientadapter.h
    llhttpconstants.h
    llhttpnode.h
    llhttpnodeadapter.h
    llhttpsender.h
    llinstantmessage.h
    llinvite.h
    lliobuffer.h
    lliohttpserver.h
    lliopipe.h
    lliosocket.h
    llioutil.h
    llloginflags.h
    llmail.h
    llmessagebuilder.h
    llmessageconfig.h
    llmessagereader.h
    llmessagetemplate.h
    llmessagetemplateparser.h
    llmessagethrottle.h
    llmsgvariabletype.h
    llnamevalue.h
    llnullcipher.h
    llpacketack.h
    llpacketbuffer.h
    llpacketring.h
    llpartdata.h
    llpumpio.h
    llproxy.h
    llqueryflags.h
    llregionflags.h
    llregionhandle.h
    llsdappservices.h
    llsdhttpserver.h
    llsdmessage.h
    llsdmessagebuilder.h
    llsdmessagereader.h
    llsdrpcclient.h
    llsdrpcserver.h
    llservice.h
    llservicebuilder.h
    llstoredmessage.h
    lltaskname.h
    llteleportflags.h
    lltemplatemessagebuilder.h
    lltemplatemessagedispatcher.h
    lltemplatemessagereader.h
    llthrottle.h
    lltransfermanager.h
    lltransfersourceasset.h
    lltransfersourcefile.h
    lltransfertargetfile.h
    lltransfertargetvfile.h
    lltrustedmessageservice.h
    llurlrequest.h
    lluseroperation.h
    llvehicleparams.h
    llxfer.h
    llxfermanager.h
    llxfer_file.h
    llxfer_mem.h
    llxfer_vfile.h
    llxorcipher.h
    machine.h
    mean_collision_data.h
    message.h
    message_prehash.h
    net.h
    partsyspacket.h
    patch_code.h
    patch_dct.h
    sound_ids.h
    )

set_source_files_properties(${llmessage_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND llmessage_SOURCE_FILES ${llmessage_HEADER_FILES})

add_library (llmessage ${llmessage_SOURCE_FILES})
target_link_libraries(
  llmessage
  ${CURL_LIBRARIES}
  ${LLCOMMON_LIBRARIES}
  ${LLVFS_LIBRARES}
  ${LLMATH_LIBRARIES}
  ${CARES_LIBRARIES}
  ${OPENSSL_LIBRARIES}
  ${CRYPTO_LIBRARIES}
  ${XMLRPCEPI_LIBRARIES}
  )

# tests
if (LL_TESTS)
  SET(llmessage_TEST_SOURCE_FILES
    llnamevalue.cpp
    lltrustedmessageservice.cpp
    lltemplatemessagedispatcher.cpp
    )
  LL_ADD_PROJECT_UNIT_TESTS(llmessage "${llmessage_TEST_SOURCE_FILES}")

  #    set(TEST_DEBUG on)
  set(test_libs
    ${CURL_LIBRARIES}
    ${LLMESSAGE_LIBRARIES}
    ${WINDOWS_LIBRARIES}
    ${LLVFS_LIBRARIES}
    ${LLMATH_LIBRARIES}
    ${LLCOMMON_LIBRARIES}
      ${GOOGLEMOCK_LIBRARIES}
    )

  LL_ADD_INTEGRATION_TEST(
    llsdmessage
    "llsdmessage.cpp"
    "${test_libs}"
    ${PYTHON_EXECUTABLE}
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_llsdmessage_peer.py"
    )

  LL_ADD_INTEGRATION_TEST(
    llhttpclient
    "llhttpclient.cpp"
    "${test_libs}"
    ${PYTHON_EXECUTABLE}
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_llsdmessage_peer.py"
    )

  LL_ADD_INTEGRATION_TEST(llavatarnamecache "" "${test_libs}")
  LL_ADD_INTEGRATION_TEST(llhost "" "${test_libs}")
  LL_ADD_INTEGRATION_TEST(llhttpclientadapter "" "${test_libs}")
  LL_ADD_INTEGRATION_TEST(llpartdata "" "${test_libs}")
  LL_ADD_INTEGRATION_TEST(llxfer_file "" "${test_libs}")
endif (LL_TESTS)

