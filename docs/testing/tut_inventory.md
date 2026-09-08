# TUT touchpoint inventory (viewer develop, sparse session 2026-09-08)

## Counts by library (*_test.cpp)
     44 indra/llcommon
     22 indra/llmessage
     21 indra/newview
     16 indra/llmath
      5 indra/llinventory
      3 indra/llprimitive
      2 indra/llui
      2 indra/llfilesystem
      1 indra/viewer_components
      1 indra/llxml
      1 indra/llkdu
      1 indra/llimage
      1 indra/llcorehttp
      1 indra/llcharacter

## Full *_test.cpp list
indra/llcharacter/tests/lljoint_test.cpp
indra/llcommon/tests/apply_test.cpp
indra/llcommon/tests/bitpack_test.cpp
indra/llcommon/tests/classic_callback_test.cpp
indra/llcommon/tests/commonmisc_test.cpp
indra/llcommon/tests/lazyeventapi_test.cpp
indra/llcommon/tests/llapp_test.cpp
indra/llcommon/tests/llbase64_test.cpp
indra/llcommon/tests/llcond_test.cpp
indra/llcommon/tests/lldate_test.cpp
indra/llcommon/tests/lldeadmantimer_test.cpp
indra/llcommon/tests/lldependencies_test.cpp
indra/llcommon/tests/lldoubledispatch_test.cpp
indra/llcommon/tests/llerror_test.cpp
indra/llcommon/tests/lleventcoro_test.cpp
indra/llcommon/tests/lleventdispatcher_test.cpp
indra/llcommon/tests/lleventfilter_test.cpp
indra/llcommon/tests/llevents_test.cpp
indra/llcommon/tests/llexception_test.cpp
indra/llcommon/tests/llfile_test.cpp
indra/llcommon/tests/llframetimer_test.cpp
indra/llcommon/tests/llheteromap_test.cpp
indra/llcommon/tests/llhttpdate_test.cpp
indra/llcommon/tests/llinstancetracker_test.cpp
indra/llcommon/tests/llleap_test.cpp
indra/llcommon/tests/llmainthreadtask_test.cpp
indra/llcommon/tests/llpounceable_test.cpp
indra/llcommon/tests/llprocess_test.cpp
indra/llcommon/tests/llprocessor_test.cpp
indra/llcommon/tests/llprocinfo_test.cpp
indra/llcommon/tests/llrand_test.cpp
indra/llcommon/tests/llsd_test.cpp
indra/llcommon/tests/llsdserialize_test.cpp
indra/llcommon/tests/llsdutil_test.cpp
indra/llcommon/tests/llsingleton_test.cpp
indra/llcommon/tests/llstreamtools_test.cpp
indra/llcommon/tests/llstring_test.cpp
indra/llcommon/tests/lltrace_test.cpp
indra/llcommon/tests/lltreeiterators_test.cpp
indra/llcommon/tests/llunits_test.cpp
indra/llcommon/tests/lluri_test.cpp
indra/llcommon/tests/stringize_test.cpp
indra/llcommon/tests/threadsafeschedule_test.cpp
indra/llcommon/tests/tuple_test.cpp
indra/llcommon/tests/workqueue_test.cpp
indra/llcorehttp/tests/llcorehttp_test.cpp
indra/llfilesystem/tests/lldir_test.cpp
indra/llfilesystem/tests/lldiriterator_test.cpp
indra/llimage/tests/llimageworker_test.cpp
indra/llinventory/tests/inventorymisc_test.cpp
indra/llinventory/tests/llparcel_test.cpp
indra/llinventory/tests/llpermissions_test.cpp
indra/llinventory/tests/llsaleinfo_test.cpp
indra/llinventory/tests/lluserrelations_test.cpp
indra/llkdu/tests/llimagej2ckdu_test.cpp
indra/llmath/tests/alignment_test.cpp
indra/llmath/tests/llbbox_test.cpp
indra/llmath/tests/llbboxlocal_test.cpp
indra/llmath/tests/llmodularmath_test.cpp
indra/llmath/tests/llquaternion_test.cpp
indra/llmath/tests/llrect_test.cpp
indra/llmath/tests/m3math_test.cpp
indra/llmath/tests/mathmisc_test.cpp
indra/llmath/tests/v2math_test.cpp
indra/llmath/tests/v3color_test.cpp
indra/llmath/tests/v3dmath_test.cpp
indra/llmath/tests/v3math_test.cpp
indra/llmath/tests/v4color_test.cpp
indra/llmath/tests/v4coloru_test.cpp
indra/llmath/tests/v4math_test.cpp
indra/llmath/tests/xform_test.cpp
indra/llmessage/tests/io_test.cpp
indra/llmessage/tests/llavatarnamecache_test.cpp
indra/llmessage/tests/llbuffer_test.cpp
indra/llmessage/tests/llcoproceduremanager_test.cpp
indra/llmessage/tests/lldatapacker_test.cpp
indra/llmessage/tests/llhost_test.cpp
indra/llmessage/tests/llhttpnode_test.cpp
indra/llmessage/tests/lliohttpserver_test.cpp
indra/llmessage/tests/llmessageconfig_test.cpp
indra/llmessage/tests/llmessagetemplateparser_test.cpp
indra/llmessage/tests/llnamevalue_test.cpp
indra/llmessage/tests/llpartdata_test.cpp
indra/llmessage/tests/llsdmessagebuilder_test.cpp
indra/llmessage/tests/llsdmessagereader_test.cpp
indra/llmessage/tests/llservicebuilder_test.cpp
indra/llmessage/tests/lltemplatemessagebuilder_test.cpp
indra/llmessage/tests/lltemplatemessagedispatcher_test.cpp
indra/llmessage/tests/lltrustedmessageservice_test.cpp
indra/llmessage/tests/llxfer_file_test.cpp
indra/llmessage/tests/llxorcipher_test.cpp
indra/llmessage/tests/llzerocode_test.cpp
indra/llmessage/tests/message_test.cpp
indra/llprimitive/tests/llgltfmaterial_test.cpp
indra/llprimitive/tests/llmediaentry_test.cpp
indra/llprimitive/tests/llprimitive_test.cpp
indra/llui/tests/llurlentry_test.cpp
indra/llui/tests/llurlmatch_test.cpp
indra/llxml/tests/llcontrol_test.cpp
indra/newview/tests/cppfeatures_test.cpp
indra/newview/tests/llagentaccess_test.cpp
indra/newview/tests/lldateutil_test.cpp
indra/newview/tests/llhttpretrypolicy_test.cpp
indra/newview/tests/lllogininstance_test.cpp
indra/newview/tests/llmediadataclient_test.cpp
indra/newview/tests/llremoteparcelrequest_test.cpp
indra/newview/tests/llsecapi_test.cpp
indra/newview/tests/llsechandler_basic_test.cpp
indra/newview/tests/llslurl_test.cpp
indra/newview/tests/lltextureinfo_test.cpp
indra/newview/tests/lltextureinfodetails_test.cpp
indra/newview/tests/llversioninfo_test.cpp
indra/newview/tests/llviewerassetstats_test.cpp
indra/newview/tests/llviewercontrollistener_test.cpp
indra/newview/tests/llviewerhelputil_test.cpp
indra/newview/tests/llviewernetwork_test.cpp
indra/newview/tests/llvocache_test.cpp
indra/newview/tests/llworldmap_test.cpp
indra/newview/tests/llworldmipmap_test.cpp
indra/newview/tests/llxmlrpclistener_test.cpp
indra/viewer_components/login/tests/lllogin_test.cpp

## Core TUT build touchpoints
- indra/cmake/Tut.cmake — use_prebuilt_binary(tut)
- indra/cmake/LLAddBuildTest.cmake — LL_ADD_PROJECT_UNIT_TESTS / LL_ADD_INTEGRATION_TEST → lltut_runner_lib
- indra/test/test.cpp, test.h, lltut.cpp, lltut.h — TUT harness + helpers
- autobuild.xml — package key `tut` (3p-tut release tarball)

## CMake registration (from develop tree)
- indra/llcommon/CMakeLists.txt — many LL_ADD_INTEGRATION_TEST + LL_ADD_PROJECT_UNIT_TESTS
- indra/llmath/CMakeLists.txt
- indra/llfilesystem, llinventory, llcorehttp, llcharacter, llmessage, llui, llxml, llprimitive, newview, ...
