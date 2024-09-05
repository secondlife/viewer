# -*- cmake -*-

project(llmessage)

include(00-Common)
include(LLAddBuildTest)
include(LLCommon)
include(LLCoreHttp)
include(LLAddBuildTest)
include(Python)
include(Tut)
include(Python)

set(llmessage_SOURCE_FILES
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
    llcoproceduremanager.cpp
    llcorehttputil.cpp
    lldatapacker.cpp
    lldispatcher.cpp
    llexperiencecache.cpp
    llgenericstreamingmessage.cpp
    llhost.cpp
    llhttpnode.cpp
    llhttpsdhandler.cpp
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
    llsdmessagebuilder.cpp
    llsdmessagereader.cpp
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
    llcoproceduremanager.h
    llcorehttputil.h
    lldatapacker.h
    lldbstrings.h
    lldispatcher.h
    lleventflags.h
    llexperiencecache.h
    llextendedstatus.h
    llfollowcamparams.h
    llgenericstreamingmessage.h
    llhost.h
    llhttpnode.h
    llhttpnodeadapter.h
    llhttpsdhandler.h
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
    llsdmessagebuilder.h
    llsdmessagereader.h
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

list(APPEND llmessage_SOURCE_FILES ${llmessage_HEADER_FILES})

add_library (llmessage ${llmessage_SOURCE_FILES})

target_link_libraries(
        llmessage
        llcommon
        llfilesystem
        llmath
        llcorehttp
)
target_include_directories( llmessage  INTERFACE   ${CMAKE_CURRENT_SOURCE_DIR})

# tests
if (LL_TESTS)
  SET(llmessage_TEST_SOURCE_FILES
    llcoproceduremanager.cpp
    llnamevalue.cpp
    lltrustedmessageservice.cpp
    lltemplatemessagedispatcher.cpp
    )
  set_property( SOURCE ${llmessage_TEST_SOURCE_FILES} PROPERTY LL_TEST_ADDITIONAL_LIBRARIES llmath llcorehttp)
  LL_ADD_PROJECT_UNIT_TESTS(llmessage "${llmessage_TEST_SOURCE_FILES}")

  #    set(TEST_DEBUG on)

  set(test_libs
          llfilesystem
          llmath
          llcorehttp
          llmessage
          llcommon
          )

  #LL_ADD_INTEGRATION_TEST(llavatarnamecache "" "${test_libs}")
  LL_ADD_INTEGRATION_TEST(llhost "" "${test_libs}")
  LL_ADD_INTEGRATION_TEST(llpartdata "" "${test_libs}")
  LL_ADD_INTEGRATION_TEST(llxfer_file "" "${test_libs}")
endif (LL_TESTS)

