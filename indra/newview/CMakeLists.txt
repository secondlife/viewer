# -*- cmake -*-

project(viewer)

include(00-Common)
include(Boost)
include(BuildVersion)
include(DBusGlib)
include(DirectX)
include(OpenSSL)
include(DragDrop)
include(EXPAT)
include(FMODEX)
include(OPENAL)
include(OpenGL)
include(Hunspell)
include(JsonCpp)
include(LLAudio)
include(LLCharacter)
include(LLCommon)
include(LLCoreHttp)
include(LLImage)
include(LLImageJ2COJ)
include(LLInventory)
include(LLMath)
include(LLMessage)
include(LLPhysicsExtensions)
include(LLPlugin)
include(LLPrimitive)
include(LLRender)
include(LLUI)
include(LLVFS)
include(LLWindow)
include(LLXML)
include(LScript)
include(Linking)
include(NDOF)
include(NVAPI)
include(GooglePerfTools)
include(TemplateCheck)
include(UI)
include(UnixInstall)
include(LLKDU)
include(ViewerMiscLibs)
include(LLLogin)
include(VisualLeakDetector)
include(GLOD)
include(CMakeCopyIfDifferent)
include(LLAppearance)

if (NOT HAVOK_TPV)
   # When using HAVOK_TPV, the library is precompiled, so no need for this
   add_subdirectory(${LLPHYSICSEXTENSIONS_SRC_DIR} llphysicsextensions)
endif (NOT HAVOK_TPV)

if(FMODEX)
  include_directories(${FMODEX_INCLUDE_DIR})
endif(FMODEX)

include_directories(
    ${DBUSGLIB_INCLUDE_DIRS}
    ${JSONCPP_INCLUDE_DIR}
    ${GLOD_INCLUDE_DIR}
    ${LLAUDIO_INCLUDE_DIRS}
    ${LLCHARACTER_INCLUDE_DIRS}
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLCOREHTTP_INCLUDE_DIRS}
    ${LLPHYSICS_INCLUDE_DIRS}
    ${LLIMAGE_INCLUDE_DIRS}
    ${LLKDU_INCLUDE_DIRS}
    ${LLINVENTORY_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
    ${LLMESSAGE_INCLUDE_DIRS}
    ${LLPLUGIN_INCLUDE_DIRS}
    ${LLPRIMITIVE_INCLUDE_DIRS}
    ${LLRENDER_INCLUDE_DIRS}
    ${LLUI_INCLUDE_DIRS}
    ${LLVFS_INCLUDE_DIRS}
    ${LLWINDOW_INCLUDE_DIRS}
    ${LLXML_INCLUDE_DIRS}
    ${LSCRIPT_INCLUDE_DIRS}
    ${LSCRIPT_INCLUDE_DIRS}/lscript_compile
    ${LLLOGIN_INCLUDE_DIRS}
    ${UPDATER_INCLUDE_DIRS}
    ${LIBS_PREBUILT_DIR}/include/collada
    ${LIBS_PREBUILD_DIR}/include/hunspell
    ${OPENAL_LIB_INCLUDE_DIRS}
    ${LIBS_PREBUILT_DIR}/include/collada/1.4
    ${LLAPPEARANCE_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
    )

include_directories(SYSTEM
    ${LLCOMMON_SYSTEM_INCLUDE_DIRS}
    ${LLXML_SYSTEM_INCLUDE_DIRS}
    ${LLPHYSICSEXTENSIONS_INCLUDE_DIRS}
    )

set(viewer_SOURCE_FILES
    groupchatlistener.cpp
    llaccountingcostmanager.cpp
    llagent.cpp
    llagentaccess.cpp
    llagentcamera.cpp
    llagentdata.cpp
    llagentlanguage.cpp
    llagentlistener.cpp
    llagentpicksinfo.cpp
    llagentpilot.cpp
    llagentui.cpp
    llagentwearables.cpp
    llagentwearablesfetch.cpp
    llanimstatelabels.cpp
    llappcorehttp.cpp
    llappearancemgr.cpp
    llappviewer.cpp
    llappviewerlistener.cpp
    llassetuploadqueue.cpp
    llassetuploadresponders.cpp
    llattachmentsmgr.cpp
    llaudiosourcevo.cpp
    llautoreplace.cpp
    llavataractions.cpp
    llavatariconctrl.cpp
    llavatarlist.cpp
    llavatarlistitem.cpp
    llavatarrenderinfoaccountant.cpp
    llavatarpropertiesprocessor.cpp
    llblockedlistitem.cpp
    llblocklist.cpp
    llbox.cpp
    llbreadcrumbview.cpp
    llbrowsernotification.cpp
    llbuycurrencyhtml.cpp
    llcallbacklist.cpp
    llcallingcard.cpp
    llcapabilitylistener.cpp
    llcaphttpsender.cpp
    llchannelmanager.cpp
    llchatbar.cpp
    llchathistory.cpp
    llchatitemscontainerctrl.cpp
    llchatmsgbox.cpp
    llchiclet.cpp
    llchicletbar.cpp
    llclassifiedinfo.cpp
    llclassifiedstatsresponder.cpp
    llcofwearables.cpp
    llcolorswatch.cpp
    llcommanddispatcherlistener.cpp
    llcommandhandler.cpp
    llcommandlineparser.cpp
    llcommunicationchannel.cpp
    llcompilequeue.cpp
    llconfirmationmanager.cpp
    llconversationlog.cpp
    llconversationloglist.cpp
    llconversationloglistitem.cpp
    llconversationmodel.cpp
    llconversationview.cpp
    llcurrencyuimanager.cpp
    llcylinder.cpp
    lldateutil.cpp
    lldaycyclemanager.cpp
    lldebugmessagebox.cpp
    lldebugview.cpp
    lldeferredsounds.cpp
    lldelayedgestureerror.cpp
    lldirpicker.cpp
    lldonotdisturbnotificationstorage.cpp
    lldndbutton.cpp
    lldrawable.cpp
    lldrawpool.cpp
    lldrawpoolalpha.cpp
    lldrawpoolavatar.cpp
    lldrawpoolbump.cpp
    lldrawpoolground.cpp
    lldrawpoolmaterials.cpp
    lldrawpoolsimple.cpp
    lldrawpoolsky.cpp
    lldrawpoolterrain.cpp
    lldrawpooltree.cpp
    lldrawpoolwater.cpp
    lldrawpoolwlsky.cpp
    lldynamictexture.cpp
    llemote.cpp
    llenvmanager.cpp
    llestateinfomodel.cpp
    lleventnotifier.cpp
    lleventpoll.cpp
    llexpandabletextbox.cpp
    llexternaleditor.cpp
    llface.cpp
    llfacebookconnect.cpp
    llfasttimerview.cpp
    llfavoritesbar.cpp
    llfeaturemanager.cpp
    llfilepicker.cpp
    llfilteredwearablelist.cpp
    llfirstuse.cpp
    llflexibleobject.cpp
    llfloaterabout.cpp
    llfloaterbvhpreview.cpp
    llfloaterauction.cpp
    llfloaterautoreplacesettings.cpp
    llfloateravatar.cpp
    llfloateravatarpicker.cpp
    llfloateravatartextures.cpp
    llfloaterbeacons.cpp
    llfloaterbuildoptions.cpp
    llfloaterbulkpermission.cpp
    llfloaterbump.cpp
    llfloaterbuy.cpp
    llfloaterbuycontents.cpp
    llfloaterbuycurrency.cpp
    llfloaterbuycurrencyhtml.cpp
    llfloaterbuyland.cpp
    llfloatercamera.cpp
    llfloaterchatvoicevolume.cpp
    llfloatercolorpicker.cpp
    llfloaterconversationlog.cpp
    llfloaterconversationpreview.cpp
    llfloaterdeleteenvpreset.cpp
    llfloaterdestinations.cpp
    llfloaterdisplayname.cpp
    llfloatereditdaycycle.cpp
    llfloatereditsky.cpp
    llfloatereditwater.cpp
    llfloaterenvironmentsettings.cpp
    llfloaterevent.cpp
    llfloaterfonttest.cpp
    llfloatergesture.cpp
    llfloatergodtools.cpp
    llfloatergotoline.cpp
    llfloatergroupinvite.cpp
    llfloatergroups.cpp
    llfloaterhandler.cpp
    llfloaterhardwaresettings.cpp
    llfloaterhelpbrowser.cpp
    llfloaterhud.cpp
    llfloaterimagepreview.cpp
    llfloaterimsessiontab.cpp
    llfloaterimsession.cpp
    llfloaterimcontainer.cpp
    llfloaterinspect.cpp
    llfloaterinventory.cpp
    llfloaterjoystick.cpp
    llfloaterland.cpp
    llfloaterlandholdings.cpp
    llfloatermap.cpp
    llfloatermediasettings.cpp
    llfloatermemleak.cpp
    llfloatermodelpreview.cpp
    llfloatermodeluploadbase.cpp
    llfloaternamedesc.cpp
    llfloaternotificationsconsole.cpp
    llfloaterobjectweights.cpp
    llfloateropenobject.cpp
    llfloateroutbox.cpp
    llfloaterpathfindingcharacters.cpp
    llfloaterpathfindingconsole.cpp
    llfloaterpathfindinglinksets.cpp
    llfloaterpathfindingobjects.cpp
    llfloaterpay.cpp
    llfloaterperms.cpp
    llfloaterpostprocess.cpp
    llfloaterpreference.cpp
    llfloaterproperties.cpp
    llfloaterregiondebugconsole.cpp
    llfloaterregioninfo.cpp
    llfloaterreporter.cpp
    llfloaterregionrestarting.cpp
    llfloatersceneloadstats.cpp
    llfloaterscriptdebug.cpp
    llfloaterscriptedprefs.cpp
    llfloaterscriptlimits.cpp
    llfloatersearch.cpp
    llfloatersellland.cpp
    llfloatersettingsdebug.cpp
    llfloatersidepanelcontainer.cpp
    llfloatersnapshot.cpp
    llfloatersocial.cpp
    llfloatersounddevices.cpp
    llfloaterspellchecksettings.cpp
    llfloatertelehub.cpp
    llfloatertestinspectors.cpp
    llfloatertestlistview.cpp
    llfloatertexturefetchdebugger.cpp
    llfloatertools.cpp
    llfloatertopobjects.cpp
    llfloatertos.cpp
    llfloatertoybox.cpp
    llfloatertranslationsettings.cpp
    llfloateruipreview.cpp
    llfloaterurlentry.cpp
    llfloatervoiceeffect.cpp
    llfloatervoicevolume.cpp
    llfloaterwebcontent.cpp
    llfloaterwebprofile.cpp
    llfloaterwhitelistentry.cpp
    llfloaterwindowsize.cpp
    llfloaterworldmap.cpp
    llfolderviewmodelinventory.cpp
    llfollowcam.cpp
    llfriendcard.cpp
    llgesturelistener.cpp
    llgesturemgr.cpp
    llgiveinventory.cpp
    llglsandbox.cpp
    llgroupactions.cpp
    llgroupiconctrl.cpp
    llgrouplist.cpp
    llgroupmgr.cpp
    llhasheduniqueid.cpp
    llhints.cpp
    llhomelocationresponder.cpp
    llhudeffect.cpp
    llhudeffectbeam.cpp
    llhudeffectlookat.cpp
    llhudeffectpointat.cpp
    llhudeffecttrail.cpp
    llhudeffectblob.cpp
    llhudicon.cpp
    llhudmanager.cpp
    llhudnametag.cpp
    llhudobject.cpp
    llhudrender.cpp
    llhudtext.cpp
    llhudview.cpp
    llimhandler.cpp
    llimview.cpp
    llinspect.cpp
    llinspectavatar.cpp
    llinspectgroup.cpp
    llinspectobject.cpp
    llinspectremoteobject.cpp
    llinspecttoast.cpp
    llinventorybridge.cpp
    llinventoryfilter.cpp
    llinventoryfunctions.cpp
    llinventoryicon.cpp
    llinventoryitemslist.cpp
    llinventorylistitem.cpp
    llinventorymodel.cpp
    llinventorymodelbackgroundfetch.cpp
    llinventoryobserver.cpp
    llinventorypanel.cpp
    lljoystickbutton.cpp
    lllandmarkactions.cpp
    lllandmarklist.cpp
    lllistbrowser.cpp
    lllistcontextmenu.cpp
    lllistview.cpp
    lllocalbitmaps.cpp
    lllocationhistory.cpp
    lllocationinputctrl.cpp
    lllogchat.cpp
    llloginhandler.cpp
    lllogininstance.cpp
    llmachineid.cpp
    llmainlooprepeater.cpp
    llmanip.cpp
    llmaniprotate.cpp
    llmanipscale.cpp
    llmaniptranslate.cpp
    llmarketplacefunctions.cpp
    llmarketplacenotifications.cpp
    llmaterialmgr.cpp
    llmediactrl.cpp
    llmediadataclient.cpp
    llmenuoptionpathfindingrebakenavmesh.cpp
    llmeshrepository.cpp
    llmimetypes.cpp
    llmorphview.cpp
    llmoveview.cpp
    llmutelist.cpp
    llnamebox.cpp
    llnameeditor.cpp
    llnamelistctrl.cpp
    llnavigationbar.cpp
    llfloaterimnearbychat.cpp
    llfloaterimnearbychathandler.cpp
    llfloaterimnearbychatlistener.cpp
    llnetmap.cpp
    llnotificationalerthandler.cpp
    llnotificationgrouphandler.cpp
    llnotificationhandlerutil.cpp
    llnotificationhinthandler.cpp
    llnotificationmanager.cpp
    llnotificationofferhandler.cpp
    llnotificationscripthandler.cpp
    llnotificationstorage.cpp
    llnotificationtiphandler.cpp
    lloutfitslist.cpp
    lloutfitobserver.cpp
    lloutputmonitorctrl.cpp
    llpanelappearancetab.cpp
    llpanelavatar.cpp
    llpanelavatartag.cpp
    llpanelblockedlist.cpp
    llpanelclassified.cpp
    llpanelcontents.cpp
    llpaneleditwearable.cpp
    llpanelface.cpp
    llpanelgenerictip.cpp
    llpanelgroup.cpp
    llpanelgroupgeneral.cpp
    llpanelgroupinvite.cpp
    llpanelgrouplandmoney.cpp
    llpanelgroupnotices.cpp
    llpanelgrouproles.cpp
    llpanelhome.cpp
    llpanelland.cpp
    llpanellandaudio.cpp
    llpanellandmarkinfo.cpp
    llpanellandmarks.cpp
    llpanellandmedia.cpp
    llpanellogin.cpp
    llpanelloginlistener.cpp
    llpanelmaininventory.cpp
    llpanelmarketplaceinbox.cpp
    llpanelmarketplaceinboxinventory.cpp
    llpanelmediasettingsgeneral.cpp
    llpanelmediasettingspermissions.cpp
    llpanelmediasettingssecurity.cpp
    llpanelme.cpp
    llpanelnearbymedia.cpp
    llpanelobject.cpp
    llpanelobjectinventory.cpp
    llpanelonlinestatus.cpp
    llpaneloutfitedit.cpp
    llpaneloutfitsinventory.cpp
    llpanelpeople.cpp
    llpanelpeoplemenus.cpp
    llpanelpermissions.cpp
    llpanelpick.cpp
    llpanelpicks.cpp
    llpanelplaceinfo.cpp
    llpanelplaceprofile.cpp
    llpanelplaces.cpp
    llpanelplacestab.cpp
    llpanelprimmediacontrols.cpp
    llpanelprofile.cpp
    llpanelsnapshot.cpp
    llpanelsnapshotinventory.cpp
    llpanelsnapshotlocal.cpp
    llpanelsnapshotoptions.cpp
    llpanelsnapshotpostcard.cpp
    llpanelsnapshotprofile.cpp
    llpanelteleporthistory.cpp
    llpaneltiptoast.cpp
    llpanelvoiceeffect.cpp
    llpaneltopinfobar.cpp
    llpanelvoicedevicesettings.cpp
    llpanelvolume.cpp
    llpanelvolumepulldown.cpp
    llpanelwearing.cpp
    llparcelselection.cpp
    llparticipantlist.cpp
    llpatchvertexarray.cpp
    llpathfindingcharacter.cpp
    llpathfindingcharacterlist.cpp
    llpathfindinglinkset.cpp
    llpathfindinglinksetlist.cpp
    llpathfindingmanager.cpp
    llpathfindingnavmesh.cpp
    llpathfindingnavmeshstatus.cpp
    llpathfindingnavmeshzone.cpp
    llpathfindingobject.cpp
    llpathfindingobjectlist.cpp
    llpathfindingpathtool.cpp
    llpersistentnotificationstorage.cpp
    llphysicsmotion.cpp
    llphysicsshapebuilderutil.cpp
    llpipelinelistener.cpp
    llplacesinventorybridge.cpp
    llplacesinventorypanel.cpp
    llplacesfolderview.cpp
    llpopupview.cpp
    llpostcard.cpp
    llpreview.cpp
    llpreviewanim.cpp
    llpreviewgesture.cpp
    llpreviewnotecard.cpp
    llpreviewscript.cpp
    llpreviewsound.cpp
    llpreviewtexture.cpp
    llproductinforequest.cpp
    llprogressview.cpp
    llrecentpeople.cpp
    llregioninfomodel.cpp
    llregionposition.cpp
    llremoteparcelrequest.cpp
    llsavedsettingsglue.cpp
    llsaveoutfitcombobtn.cpp
    llscenemonitor.cpp
    llsceneview.cpp
    llscreenchannel.cpp
    llscripteditor.cpp
    llscriptfloater.cpp
    llscrollingpanelparam.cpp
    llscrollingpanelparambase.cpp
    llsearchcombobox.cpp
    llsearchhistory.cpp
    llsecapi.cpp
    llsechandler_basic.cpp
    llselectmgr.cpp
    llshareavatarhandler.cpp
    llsidepanelappearance.cpp
    llsidepanelinventory.cpp
    llsidepanelinventorysubpanel.cpp
    llsidepaneliteminfo.cpp
    llsidepaneltaskinfo.cpp
    llsidetraypanelcontainer.cpp
    llsky.cpp
    llslurl.cpp
    llsnapshotlivepreview.cpp
    llspatialpartition.cpp
    llspeakers.cpp
    llspeakingindicatormanager.cpp
    llsplitbutton.cpp
    llsprite.cpp
    llstartup.cpp
    llstartuplistener.cpp
    llstatusbar.cpp
    llstylemap.cpp
    llsurface.cpp
    llsurfacepatch.cpp
    llsyntaxid.cpp
    llsyswellitem.cpp
    llsyswellwindow.cpp
    llteleporthistory.cpp
    llteleporthistorystorage.cpp
    lltextureatlas.cpp
    lltextureatlasmanager.cpp
    lltexturecache.cpp
    lltexturectrl.cpp
    lltexturefetch.cpp
    lltextureinfo.cpp
    lltextureinfodetails.cpp
    lltexturestats.cpp
    lltexturestatsuploader.cpp
    lltextureview.cpp
    lltoast.cpp
    lltoastalertpanel.cpp
    lltoastgroupnotifypanel.cpp
    lltoastimpanel.cpp
    lltoastnotifypanel.cpp
    lltoastpanel.cpp
    lltoastscripttextbox.cpp
    lltoastscriptquestion.cpp
    lltool.cpp
    lltoolbarview.cpp
    lltoolbrush.cpp
    lltoolcomp.cpp
    lltooldraganddrop.cpp
    lltoolface.cpp
    lltoolfocus.cpp
    lltoolgrab.cpp
    lltoolgun.cpp
    lltoolindividual.cpp
    lltoolmgr.cpp
    lltoolmorph.cpp
    lltoolobjpicker.cpp
    lltoolpie.cpp
    lltoolpipette.cpp
    lltoolplacer.cpp
    lltoolselect.cpp
    lltoolselectland.cpp
    lltoolselectrect.cpp
    lltracker.cpp
    lltransientdockablefloater.cpp
    lltransientfloatermgr.cpp
    lltranslate.cpp
    lluilistener.cpp
    lluploaddialog.cpp
    lluploadfloaterobservers.cpp
    llurl.cpp
    llurldispatcher.cpp
    llurldispatcherlistener.cpp
    llurlhistory.cpp
    llurllineeditorctrl.cpp
    llurlwhitelist.cpp
    llvectorperfoptions.cpp
    llversioninfo.cpp
    llviewchildren.cpp
    llviewerassetstats.cpp
    llviewerassetstorage.cpp
    llviewerassettype.cpp
    llviewerattachmenu.cpp
    llvieweraudio.cpp
    llviewercamera.cpp
    llviewerchat.cpp
    llviewercontrol.cpp
    llviewercontrollistener.cpp
    llviewerdisplay.cpp
    llviewerdisplayname.cpp
    llviewerfloaterreg.cpp
    llviewerfoldertype.cpp
    llviewergenericmessage.cpp
    llviewergesture.cpp
    llviewerhelp.cpp
    llviewerhelputil.cpp
    llviewerhome.cpp
    llviewerinventory.cpp
    llviewerjoint.cpp
    llviewerjointattachment.cpp
    llviewerjointmesh.cpp
    llviewerjoystick.cpp
    llviewerkeyboard.cpp
    llviewerlayer.cpp
    llviewermedia.cpp
    llviewermedia_streamingaudio.cpp
    llviewermediafocus.cpp
    llviewermenu.cpp
    llviewermenufile.cpp
    llviewermessage.cpp
    llviewernetwork.cpp
    llviewerobject.cpp
    llviewerobjectlist.cpp
    llvieweroctree.cpp
    llviewerparcelmedia.cpp
    llviewerparcelmediaautoplay.cpp
    llviewerparcelmgr.cpp
    llviewerparceloverlay.cpp
    llviewerpartsim.cpp
    llviewerpartsource.cpp
    llviewerregion.cpp
    llviewershadermgr.cpp
    llviewerstats.cpp
    llviewerstatsrecorder.cpp
    llviewertexlayer.cpp
    llviewertexteditor.cpp
    llviewertexture.cpp
    llviewertextureanim.cpp
    llviewertexturelist.cpp
    llviewerthrottle.cpp
    llviewerwearable.cpp
    llviewerwindow.cpp
    llviewerwindowlistener.cpp
    llvlcomposition.cpp
    llvlmanager.cpp
    llvoavatar.cpp
    llvoavatarself.cpp
    llvocache.cpp
    llvograss.cpp
    llvoground.cpp
    llvoicecallhandler.cpp
    llvoicechannel.cpp
    llvoiceclient.cpp
    llvoicevisualizer.cpp
    llvoicevivox.cpp
    llvoinventorylistener.cpp
    llvopartgroup.cpp
    llvosky.cpp
    llvosurfacepatch.cpp
    llvotree.cpp
    llvovolume.cpp
    llvowater.cpp
    llvowlsky.cpp
    llwatchdog.cpp
    llwaterparammanager.cpp
    llwaterparamset.cpp
    llwearableitemslist.cpp
    llwearablelist.cpp
    llweb.cpp
    llwebprofile.cpp
    llwebsharing.cpp
    llwind.cpp
    llwindowlistener.cpp
    llwlanimator.cpp
    llwldaycycle.cpp
    llwlhandlers.cpp
    llwlparammanager.cpp
    llwlparamset.cpp
    llworld.cpp
    llworldmap.cpp
    llworldmapmessage.cpp
    llworldmipmap.cpp
    llworldmapview.cpp
    llxmlrpclistener.cpp
    llxmlrpctransaction.cpp
    noise.cpp
    pipeline.cpp
    )

set(VIEWER_BINARY_NAME "secondlife-bin" CACHE STRING
    "The name of the viewer executable to create.")

set(viewer_HEADER_FILES
    CMakeLists.txt
    ViewerInstall.cmake
    groupchatlistener.h
    llaccountingcost.h
    llaccountingcostmanager.h
    llagent.h
    llagentaccess.h
    llagentcamera.h
    llagentdata.h
    llagentlanguage.h
    llagentlistener.h
    llagentpicksinfo.h
    llagentpilot.h
    llagentui.h
    llagentwearables.h
    llagentwearablesfetch.h
    llanimstatelabels.h
    llappcorehttp.h
    llappearance.h
    llappearancemgr.h
    llappviewer.h
    llappviewerlistener.h
    llassetuploadqueue.h
    llassetuploadresponders.h
    llattachmentsmgr.h
    llaudiosourcevo.h
    llautoreplace.h
    llavataractions.h
    llavatariconctrl.h
    llavatarlist.h
    llavatarlistitem.h
    llavatarpropertiesprocessor.h
    llavatarrenderinfoaccountant.h
    llblockedlistitem.h
    llblocklist.h
    llbox.h
    llbreadcrumbview.h
    llbuycurrencyhtml.h
    llcallbacklist.h
    llcallingcard.h
    llcapabilitylistener.h
    llcapabilityprovider.h
    llcaphttpsender.h
    llchannelmanager.h
    llchatbar.h
    llchathistory.h
    llchatitemscontainerctrl.h
    llchatmsgbox.h
    llchiclet.h
    llchicletbar.h
    llclassifiedinfo.h
    llclassifiedstatsresponder.h
    llcofwearables.h
    llcolorswatch.h
    llcommanddispatcherlistener.h
    llcommandhandler.h
    llcommandlineparser.h
    llcommunicationchannel.h
    llcompilequeue.h
    llconfirmationmanager.h
    llconversationlog.h
    llconversationloglist.h
    llconversationloglistitem.h
    llconversationmodel.h
    llconversationview.h
    llcurrencyuimanager.h
    llcylinder.h
    lldateutil.h
    lldaycyclemanager.h
    lldebugmessagebox.h
    lldebugview.h
    lldeferredsounds.h
    lldelayedgestureerror.h
    lldirpicker.h
    lldonotdisturbnotificationstorage.h
    lldndbutton.h
    lldrawable.h
    lldrawpool.h
    lldrawpoolalpha.h
    lldrawpoolavatar.h
    lldrawpoolbump.h
    lldrawpoolmaterials.h
    lldrawpoolground.h
    lldrawpoolsimple.h
    lldrawpoolsky.h
    lldrawpoolterrain.h
    lldrawpooltree.h
    lldrawpoolwater.h
    lldrawpoolwlsky.h
    lldynamictexture.h
    llemote.h
    llenvmanager.h
    llestateinfomodel.h
    lleventnotifier.h
    lleventpoll.h
    llexpandabletextbox.h
    llexternaleditor.h
    llface.h
    llfacebookconnect.h
    llfasttimerview.h
    llfavoritesbar.h
    llfeaturemanager.h
    llfilepicker.h
    llfilteredwearablelist.h
    llfirstuse.h
    llflexibleobject.h
    llfloaterabout.h
    llfloaterbvhpreview.h
    llfloaterauction.h
    llfloaterautoreplacesettings.h
    llfloateravatar.h
    llfloateravatarpicker.h
    llfloateravatartextures.h
    llfloaterbeacons.h
    llfloaterbuildoptions.h
    llfloaterbulkpermission.h
    llfloaterbump.h
    llfloaterbuy.h
    llfloaterbuycontents.h
    llfloaterbuycurrency.h
    llfloaterbuycurrencyhtml.h
    llfloaterbuyland.h
    llfloatercamera.h
    llfloaterchatvoicevolume.h
    llfloatercolorpicker.h
    llfloaterconversationlog.h
    llfloaterconversationpreview.h
    llfloaterdeleteenvpreset.h
    llfloaterdestinations.h
    llfloaterdisplayname.h
    llfloatereditdaycycle.h
    llfloatereditsky.h
    llfloatereditwater.h
    llfloaterenvironmentsettings.h
    llfloaterevent.h
    llfloaterfonttest.h
    llfloatergesture.h
    llfloatergodtools.h
    llfloatergotoline.h
    llfloatergroupinvite.h
    llfloatergroups.h
    llfloaterhandler.h
    llfloaterhardwaresettings.h
    llfloaterhelpbrowser.h
    llfloaterhud.h
    llfloaterimagepreview.h
    llfloaterimnearbychat.h
    llfloaterimnearbychathandler.h
    llfloaterimnearbychatlistener.h
    llfloaterimsessiontab.h
    llfloaterimsession.h
    llfloaterimcontainer.h
    llfloaterinspect.h
    llfloaterinventory.h
    llfloaterjoystick.h
    llfloaterland.h
    llfloaterlandholdings.h
    llfloatermap.h
    llfloatermediasettings.h
    llfloatermemleak.h
    llfloatermodelpreview.h
    llfloatermodeluploadbase.h
    llfloaternamedesc.h
    llfloaternotificationsconsole.h
    llfloaterobjectweights.h
    llfloateropenobject.h
    llfloateroutbox.h
    llfloaterpathfindingcharacters.h
    llfloaterpathfindingconsole.h
    llfloaterpathfindinglinksets.h
    llfloaterpathfindingobjects.h
    llfloaterpay.h
    llfloaterperms.h
    llfloaterpostprocess.h
    llfloaterpreference.h
    llfloaterproperties.h
    llfloaterregiondebugconsole.h
    llfloaterregioninfo.h
    llfloaterreporter.h
    llfloaterregionrestarting.h
    llfloatersceneloadstats.h
    llfloaterscriptdebug.h
    llfloaterscriptedprefs.h
    llfloaterscriptlimits.h
    llfloatersearch.h
    llfloatersellland.h
    llfloatersettingsdebug.h
    llfloatersidepanelcontainer.h
    llfloatersnapshot.h
    llfloatersocial.h
    llfloatersounddevices.h
    llfloaterspellchecksettings.h
    llfloatertelehub.h
    llfloatertestinspectors.h
    llfloatertestlistview.h
    llfloatertexturefetchdebugger.h
    llfloatertools.h
    llfloatertopobjects.h
    llfloatertos.h
    llfloatertoybox.h
    llfloatertranslationsettings.h
    llfloateruipreview.h
    llfloaterurlentry.h
    llfloatervoiceeffect.h
    llfloatervoicevolume.h
    llfloaterwebcontent.h
    llfloaterwebprofile.h
    llfloaterwhitelistentry.h
    llfloaterwindowsize.h
    llfloaterworldmap.h
    llfolderviewmodelinventory.h
    llfollowcam.h
    llfriendcard.h
    llgesturelistener.h
    llgesturemgr.h
    llgiveinventory.h
    llgroupactions.h
    llgroupiconctrl.h
    llgrouplist.h
    llgroupmgr.h
    llhasheduniqueid.h
    llhints.h
    llhomelocationresponder.h
    llhudeffect.h
    llhudeffectbeam.h
    llhudeffectlookat.h
    llhudeffectpointat.h
    llhudeffecttrail.h
    llhudeffectblob.h
    llhudicon.h
    llhudmanager.h
    llhudnametag.h
    llhudobject.h
    llhudrender.h
    llhudtext.h
    llhudview.h
    llimview.h
    llinspect.h
    llinspectavatar.h
    llinspectgroup.h
    llinspectobject.h
    llinspectremoteobject.h
    llinspecttoast.h
    llinventorybridge.h
    llinventoryfilter.h
    llinventoryfunctions.h
    llinventoryicon.h
    llinventoryitemslist.h
    llinventorylistitem.h
    llinventorymodel.h
    llinventorymodelbackgroundfetch.h
    llinventoryobserver.h
    llinventorypanel.h
    lljoystickbutton.h
    lllandmarkactions.h
    lllandmarklist.h
    lllightconstants.h
    lllistbrowser.h
    lllistcontextmenu.h
    lllistview.h
    lllocalbitmaps.h
    lllocationhistory.h
    lllocationinputctrl.h
    lllogchat.h
    llloginhandler.h
    lllogininstance.h
    llmachineid.h
    llmainlooprepeater.h
    llmanip.h
    llmaniprotate.h
    llmanipscale.h
    llmaniptranslate.h
    llmarketplacefunctions.h
    llmarketplacenotifications.h
    llmaterialmgr.h
    llmediactrl.h
    llmediadataclient.h
    llmenuoptionpathfindingrebakenavmesh.h
    llmeshrepository.h
    llmimetypes.h
    llmorphview.h
    llmoveview.h
    llmutelist.h
    llnamebox.h
    llnameeditor.h
    llnamelistctrl.h
    llnavigationbar.h
    llnetmap.h
    llnotificationhandler.h
    llnotificationmanager.h
    llnotificationstorage.h
    lloutfitslist.h
    lloutfitobserver.h
    lloutputmonitorctrl.h
    llpanelappearancetab.h
    llpanelavatar.h
    llpanelavatartag.h
    llpanelblockedlist.h
    llpanelclassified.h
    llpanelcontents.h
    llpaneleditwearable.h
    llpanelface.h
    llpanelgenerictip.h
    llpanelgroup.h
    llpanelgroupgeneral.h
    llpanelgroupinvite.h
    llpanelgrouplandmoney.h
    llpanelgroupnotices.h
    llpanelgrouproles.h
    llpanelhome.h
    llpanelland.h
    llpanellandaudio.h
    llpanellandmarkinfo.h
    llpanellandmarks.h
    llpanellandmedia.h
    llpanellogin.h
    llpanelloginlistener.h
    llpanelmaininventory.h
    llpanelmarketplaceinbox.h
    llpanelmarketplaceinboxinventory.h
    llpanelmediasettingsgeneral.h
    llpanelmediasettingspermissions.h
    llpanelmediasettingssecurity.h
    llpanelme.h
    llpanelnearbymedia.h
    llpanelobject.h
    llpanelobjectinventory.h
    llpanelonlinestatus.h
    llpaneloutfitedit.h
    llpaneloutfitsinventory.h
    llpanelpeople.h
    llpanelpeoplemenus.h
    llpanelpermissions.h
    llpanelpick.h
    llpanelpicks.h
    llpanelplaceinfo.h
    llpanelplaceprofile.h
    llpanelplaces.h
    llpanelplacestab.h
    llpanelprimmediacontrols.h
    llpanelprofile.h
    llpanelsnapshot.h
    llpanelteleporthistory.h
    llpaneltiptoast.h
    llpanelvoicedevicesettings.h
    llpanelvoiceeffect.h
    llpaneltopinfobar.h
    llpanelvolume.h
    llpanelvolumepulldown.h
    llpanelwearing.h
    llparcelselection.h
    llparticipantlist.h
    llpatchvertexarray.h
    llpathfindingcharacter.h
    llpathfindingcharacterlist.h
    llpathfindinglinkset.h
    llpathfindinglinksetlist.h
    llpathfindingmanager.h
    llpathfindingnavmesh.h
    llpathfindingnavmeshstatus.h
    llpathfindingnavmeshzone.h
    llpathfindingobject.h
    llpathfindingobjectlist.h
    llpathfindingpathtool.h
    llpersistentnotificationstorage.h
    llphysicsmotion.h
    llphysicsshapebuilderutil.h
    llpipelinelistener.h
    llplacesinventorybridge.h
    llplacesinventorypanel.h
    llplacesfolderview.h
    llpopupview.h
    llpostcard.h
    llpreview.h
    llpreviewanim.h
    llpreviewgesture.h
    llpreviewnotecard.h
    llpreviewscript.h
    llpreviewsound.h
    llpreviewtexture.h
    llproductinforequest.h
    llprogressview.h
    llrecentpeople.h
    llregioninfomodel.h
    llregionposition.h
    llremoteparcelrequest.h
    llresourcedata.h
    llrootview.h
    llsavedsettingsglue.h
    llsaveoutfitcombobtn.h
    llscenemonitor.h
    llsceneview.h
    llscreenchannel.h
    llscripteditor.h
    llscriptfloater.h
    llscrollingpanelparam.h
    llscrollingpanelparambase.h
    llsearchcombobox.h
    llsearchhistory.h
    llsecapi.h
    llsechandler_basic.h
    llselectmgr.h
    llsidepanelappearance.h
    llsidepanelinventory.h
    llsidepanelinventorysubpanel.h
    llsidepaneliteminfo.h
    llsidepaneltaskinfo.h
    llsidetraypanelcontainer.h
    llsky.h
    llslurl.h
    llsnapshotlivepreview.h
    llspatialpartition.h
    llspeakers.h
    llspeakingindicatormanager.h
    llsplitbutton.h
    llsprite.h
    llstartup.h
    llstartuplistener.h
    llstatusbar.h
    llstylemap.h
    llsurface.h
    llsurfacepatch.h
    llsyntaxid.h
    llsyswellitem.h
    llsyswellwindow.h
    lltable.h
    llteleporthistory.h
    llteleporthistorystorage.h
    lltextureatlas.h
    lltextureatlasmanager.h
    lltexturecache.h
    lltexturectrl.h
    lltexturefetch.h
    lltextureinfo.h
    lltextureinfodetails.h
    lltexturestats.h
    lltexturestatsuploader.h
    lltextureview.h
    lltoast.h
    lltoastalertpanel.h
    lltoastgroupnotifypanel.h
    lltoastimpanel.h
    lltoastnotifypanel.h
    lltoastpanel.h
    lltoastscripttextbox.h
    lltoastscriptquestion.h
    lltool.h
    lltoolbarview.h
    lltoolbrush.h
    lltoolcomp.h
    lltooldraganddrop.h
    lltoolface.h
    lltoolfocus.h
    lltoolgrab.h
    lltoolgun.h
    lltoolindividual.h
    lltoolmgr.h
    lltoolmorph.h
    lltoolobjpicker.h
    lltoolpie.h
    lltoolpipette.h
    lltoolplacer.h
    lltoolselect.h
    lltoolselectland.h
    lltoolselectrect.h
    lltracker.h
    lltransientdockablefloater.h
    lltransientfloatermgr.h
    lltranslate.h
    lluiconstants.h
    lluilistener.h
    lluploaddialog.h
    lluploadfloaterobservers.h
    llurl.h
    llurldispatcher.h
    llurldispatcherlistener.h
    llurlhistory.h
    llurllineeditorctrl.h
    llurlwhitelist.h
    llvectorperfoptions.h
    llversioninfo.h
    llviewchildren.h
    llviewerassetstats.h
    llviewerassetstorage.h
    llviewerassettype.h
    llviewerattachmenu.h
    llvieweraudio.h
    llviewercamera.h
    llviewerchat.h
    llviewercontrol.h
    llviewercontrollistener.h
    llviewerdisplay.h
    llviewerdisplayname.h
    llviewerfloaterreg.h
    llviewerfoldertype.h
    llviewergenericmessage.h
    llviewergesture.h
    llviewerhelp.h
    llviewerhome.h
    llviewerinventory.h
    llviewerjoint.h
    llviewerjointattachment.h
    llviewerjointmesh.h
    llviewerjoystick.h
    llviewerkeyboard.h
    llviewerlayer.h
    llviewermedia.h
    llviewermediafocus.h
    llviewermediaobserver.h
    llviewermenu.h
    llviewermenufile.h
    llviewermessage.h
    llviewernetwork.h
    llviewerobject.h
    llviewerobjectlist.h
    llvieweroctree.h
    llviewerparcelmedia.h
    llviewerparcelmediaautoplay.h
    llviewerparcelmgr.h
    llviewerparceloverlay.h
    llviewerpartsim.h
    llviewerpartsource.h
    llviewerprecompiledheaders.h
    llviewerregion.h
    llviewershadermgr.h
    llviewerstats.h
    llviewerstatsrecorder.h
    llviewertexlayer.h
    llviewertexteditor.h
    llviewertexture.h
    llviewertextureanim.h
    llviewertexturelist.h
    llviewerthrottle.h
    llviewerwearable.h
    llviewerwindow.h
    llviewerwindowlistener.h
    llvlcomposition.h
    llvlmanager.h
    llvoavatar.h
    llvoavatarself.h
    llvocache.h
    llvograss.h
    llvoground.h
    llvoicechannel.h
    llvoiceclient.h
    llvoicevisualizer.h
    llvoicevivox.h
    llvoinventorylistener.h
    llvopartgroup.h
    llvosky.h
    llvosurfacepatch.h
    llvotree.h
    llvovolume.h
    llvowater.h
    llvowlsky.h
    llwatchdog.h
    llwaterparammanager.h
    llwaterparamset.h
    llwearableitemslist.h
    llwearablelist.h
    llweb.h
    llwebprofile.h
    llwebsharing.h
    llwind.h
    llwindowlistener.h
    llwlanimator.h
    llwldaycycle.h
    llwlhandlers.h
    llwlparammanager.h
    llwlparamset.h
    llworld.h
    llworldmap.h
    llworldmapmessage.h
    llworldmipmap.h
    llworldmapview.h
    llxmlrpclistener.h
    llxmlrpctransaction.h
    macmain.h
    noise.h
    pipeline.h
    roles_constants.h
    VertexCache.h
    VorbisFramework.h
    )

source_group("CMake Rules" FILES ViewerInstall.cmake)

# the viewer_version.txt file created here is for passing to viewer_manifest
# the summary.json file is created for the benefit of the TeamCity builds, where
#   it is used to provide descriptive information to the build results page
add_custom_target(generate_viewer_version ALL
                  COMMAND printf '${VIEWER_SHORT_VERSION}.${VIEWER_VERSION_REVISION}' > ${CMAKE_CURRENT_BINARY_DIR}/viewer_version.txt
                  COMMAND printf '{"Type":"viewer","Version":"${VIEWER_SHORT_VERSION}.${VIEWER_VERSION_REVISION}"}' > ${CMAKE_BINARY_DIR}/summary.json
                  COMMENT Generating viewer_version.txt for manifest processing
                  )

set_source_files_properties(
   llversioninfo.cpp tests/llversioninfo_test.cpp 
   PROPERTIES
   DEPENDS generate_viewer_version  # dummy dependency to force recompile every time
   COMPILE_DEFINITIONS "${VIEWER_CHANNEL_VERSION_DEFINES}" # see BuildVersion.cmake
   )

if (DARWIN)
  LIST(APPEND viewer_SOURCE_FILES llappviewermacosx.cpp)
  LIST(APPEND viewer_SOURCE_FILES llappviewermacosx-objc.mm)
  LIST(APPEND viewer_SOURCE_FILES llappviewermacosx-objc.h)
  LIST(APPEND viewer_SOURCE_FILES llfilepicker_mac.mm)
  LIST(APPEND viewer_HEADER_FILES llfilepicker_mac.h)

  # This should be compiled with the viewer.
  LIST(APPEND viewer_SOURCE_FILES llappdelegate-objc.mm)

  find_library(AGL_LIBRARY AGL)
  find_library(APPKIT_LIBRARY AppKit)
  find_library(COCOA_LIBRARY Cocoa)
  find_library(IOKIT_LIBRARY IOKit)
  find_library(COREAUDIO_LIBRARY CoreAudio)

  set(viewer_LIBRARIES
    ${COCOA_LIBRARY}
    ${AGL_LIBRARY}
    ${IOKIT_LIBRARY}
    ${COREAUDIO_LIBRARY}
    )

  # Add resource files to the project.
  set(viewer_RESOURCE_FILES
    secondlife.icns
    macview.r
    gpu_table.txt
    Info-SecondLife.plist
    SecondLife.xib/
    # CMake doesn't seem to support Xcode language variants well just yet
    English.lproj/InfoPlist.strings
    English.lproj/language.txt
    German.lproj/language.txt
    Japanese.lproj/language.txt
    Korean.lproj/language.txt
    )
  set_source_files_properties(
    ${viewer_RESOURCE_FILES}
    PROPERTIES
    HEADER_FILE_ONLY TRUE
    #MACOSX_PACKAGE_LOCATION Resources #don't do this! this tells cmake to copy the files.
    )
  SOURCE_GROUP("Resources" FILES ${viewer_RESOURCE_FILES})
  list(APPEND viewer_SOURCE_FILES ${viewer_RESOURCE_FILES})
endif (DARWIN)

if (LINUX)
    LIST(APPEND viewer_SOURCE_FILES llappviewerlinux.cpp)
    LIST(APPEND viewer_SOURCE_FILES llappviewerlinux_api_dbus.cpp)
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--as-needed")

    set(viewer_LIBRARIES
        Xinerama
        )
endif (LINUX)

if (WINDOWS)
    list(APPEND viewer_SOURCE_FILES
         llappviewerwin32.cpp
         llwindebug.cpp
         )

    list(APPEND viewer_HEADER_FILES
         llappviewerwin32.h
         llwindebug.h
         )

    # precompiled header configuration
    # llviewerprecompiledheaders.cpp generates
    # the .pch file.
    # All sources added to viewer_SOURCE_FILES
    # at this point use it.
    if(USE_PRECOMPILED_HEADERS)
        set_source_files_properties(llviewerprecompiledheaders.cpp
            PROPERTIES
            COMPILE_FLAGS "/Ycllviewerprecompiledheaders.h"
            )
        set(viewer_SOURCE_FILES "${viewer_SOURCE_FILES}" llviewerprecompiledheaders.cpp)
    endif(USE_PRECOMPILED_HEADERS)

    # Replace the icons with the appropriate ones for the channel
    # ('test' is the default)
    set(ICON_PATH "test")
    set(VIEWER_MACOSX_PHASE "d")
    string(TOLOWER ${VIEWER_CHANNEL} channel_lower)
    if(channel_lower MATCHES "^second life release")
        set(ICON_PATH "release")
        set(VIEWER_MACOSX_PHASE "f")
    elseif(channel_lower MATCHES "^second life beta")
        set(ICON_PATH "beta")
        set(VIEWER_MACOSX_PHASE "b")
    elseif(channel_lower MATCHES "^second life project")
        set(ICON_PATH "project")
        set(VIEWER_MACOSX_PHASE "a")
    endif()
    message("Copying icons for ${ICON_PATH}")
    execute_process(
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/icons/${ICON_PATH}/secondlife.ico"
        "${CMAKE_CURRENT_SOURCE_DIR}/res/ll_icon.ico"
      )
    execute_process(
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/icons/${ICON_PATH}/secondlife_256.BMP"
        "${CMAKE_CURRENT_SOURCE_DIR}/res/ll_icon.BMP"
      )
    execute_process(
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/icons/${ICON_PATH}/secondlife_256.BMP"
        "${CMAKE_CURRENT_SOURCE_DIR}/res-sdl/ll_icon.BMP"
      )

    # Add resource files to the project.
    # viewerRes.rc is the only buildable file, but
    # the rest are all dependencies of it.
    set(viewer_RESOURCE_FILES
        res/arrow.cur
        res/arrowcop.cur
        res/arrowcopmulti.cur
        res/arrowdrag.cur
        res/circleandline.cur
        res/icon1.ico
        res/llarrow.cur
        res/llarrowdrag.cur
        res/llarrowdragmulti.cur
        res/llarrowlocked.cur
        res/llgrablocked.cur
        res/llno.cur
        res/llnolocked.cur
        res/lltoolcamera.cur
        res/lltoolcreate.cur
        res/lltoolfocus.cur
        res/lltoolgrab.cur
        res/lltoolland.cur
        res/lltoolpan.cur
        res/lltoolpathfinding.cur
        res/lltoolpathfindingpathend.cur
        res/lltoolpathfindingpathendadd.cur
        res/lltoolpathfindingpathstart.cur
        res/lltoolpathfindingpathstartadd.cur
        res/lltoolpipette.cur
        res/lltoolrotate.cur
        res/lltoolscale.cur
        res/lltooltranslate.cur
        res/lltoolzoomin.cur
        res/lltoolzoomout.cur
        res-sdl/ll_icon.BMP
        res/ll_icon.BMP
        res/ll_icon.ico
        res/resource.h
        res/toolpickobject.cur
        res/toolpickobject2.cur
        res/toolpickobject3.cur
        res/toolpipette.cur
        res/toolbuy.cur
        res/toolopen.cur
        res/toolsit.cur
        )

    set_source_files_properties(${viewer_RESOURCE_FILES}
                                PROPERTIES HEADER_FILE_ONLY TRUE)

    configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/res/viewerRes.rc
                    ${CMAKE_CURRENT_BINARY_DIR}/viewerRes.rc
                    )
    set(viewer_RESOURCE_FILES
        ${CMAKE_CURRENT_BINARY_DIR}/viewerRes.rc
        ${viewer_RESOURCE_FILES}
        )

    set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/viewerRes.rc
      PROPERTIES COMPILE_FLAGS "-I${CMAKE_CURRENT_SOURCE_DIR}/res"
      )

    SOURCE_GROUP("Resource Files" FILES ${viewer_RESOURCE_FILES})

    if (NOT USESYSTEMLIBS)
        list(APPEND viewer_SOURCE_FILES ${viewer_RESOURCE_FILES})
    endif (NOT USESYSTEMLIBS)

    find_library(DINPUT_LIBRARY dinput8 ${DIRECTX_LIBRARY_DIR})
    find_library(DXGUID_LIBRARY dxguid ${DIRECTX_LIBRARY_DIR})
    mark_as_advanced(
        DINPUT_LIBRARY
        DXGUID_LIBRARY
        )

# see EXP-1765 - theory is opengl32.lib needs to be included before gdi32.lib (windows libs)
    set(viewer_LIBRARIES
        opengl32
        ${WINDOWS_LIBRARIES}
        comdlg32
        ${DINPUT_LIBRARY}
        ${DXGUID_LIBRARY}
        kernel32
        odbc32
        odbccp32
        ole32
        oleaut32
        shell32
        Vfw32
        winspool
        )

    find_library(INTEL_MEMOPS_LIBRARY
                 NAMES ll_intel_memops
                 PATHS
                 optimized ${ARCH_PREBUILT_DIRS_RELEASE}
                 debug ${ARCH_PREBUILT_DIRS_DEBUG}
                 )
    mark_as_advanced(INTEL_MEMOPS_LIBRARY)

    if (INTEL_MEMOPS_LIBRARY)
      list(APPEND viewer_LIBRARIES ${INTEL_MEMOPS_LIBRARY})
    endif (INTEL_MEMOPS_LIBRARY)
endif (WINDOWS)

# Add the xui files. This is handy for searching for xui elements
# from within the IDE.
set(viewer_XUI_FILES
    skins/default/colors.xml
    skins/default/textures/textures.xml
    )
file(GLOB DEFAULT_XUI_FILE_GLOB_LIST
     ${CMAKE_CURRENT_SOURCE_DIR}/skins/*/xui/en/*.xml)
list(APPEND viewer_XUI_FILES ${DEFAULT_XUI_FILE_GLOB_LIST})

file(GLOB DEFAULT_WIDGET_FILE_GLOB_LIST
     ${CMAKE_CURRENT_SOURCE_DIR}/skins/*/xui/en/widgets/*.xml)
list(APPEND viewer_XUI_FILES ${DEFAULT_WIDGET_FILE_GLOB_LIST})

# Cannot append empty lists in CMake, wait until we have files here.
#file(GLOB SILVER_WIDGET_FILE_GLOB_LIST
#     ${CMAKE_CURRENT_SOURCE_DIR}/skins/silver/xui/en-us/widgets/*.xml)
#list(APPEND viewer_XUI_FILES ${SILVER_WIDGET_FILE_GLOB_LIST})

list(SORT viewer_XUI_FILES)

source_group("XUI Files" FILES ${viewer_XUI_FILES})

set_source_files_properties(${viewer_XUI_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND viewer_SOURCE_FILES ${viewer_XUI_FILES})

set(viewer_APPSETTINGS_FILES
    app_settings/anim.ini
    app_settings/cmd_line.xml
    app_settings/commands.xml
    app_settings/grass.xml
    app_settings/high_graphics.xml
    app_settings/ignorable_dialogs.xml
    app_settings/keys.xml
    app_settings/keywords_lsl_default.xml
    app_settings/logcontrol.xml
    app_settings/low_graphics.xml
    app_settings/mid_graphics.xml
    app_settings/settings.xml
    app_settings/settings_crash_behavior.xml
    app_settings/settings_files.xml
    app_settings/settings_per_account.xml
    app_settings/std_bump.ini
    app_settings/toolbars.xml
    app_settings/trees.xml
    app_settings/ultra_graphics.xml
    app_settings/viewerart.xml
    ${CMAKE_SOURCE_DIR}/../etc/message.xml
    ${CMAKE_SOURCE_DIR}/../scripts/messages/message_template.msg
    )

source_group("App Settings" FILES ${viewer_APPSETTINGS_FILES})

set_source_files_properties(${viewer_APPSETTINGS_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND viewer_SOURCE_FILES ${viewer_APPSETTINGS_FILES})

set(viewer_CHARACTER_FILES
    character/attentions.xml
    character/attentionsN.xml
    character/avatar_lad.xml
    character/avatar_skeleton.xml
    character/genepool.xml
    )

source_group("Character File" FILES ${viewer_CHARACTER_FILES})

set_source_files_properties(${viewer_CHARACTER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)
if (NOT USESYSTEMLIBS)
    list(APPEND viewer_SOURCE_FILES ${viewer_CHARACTER_FILES})
endif (NOT USESYSTEMLIBS)

if (WINDOWS)
  file(GLOB viewer_INSTALLER_FILES installers/windows/*.nsi)

  source_group("Installer Files" FILES ${viewer_INSTALLER_FILES})

  set_source_files_properties(${viewer_INSTALLER_FILES}
                              PROPERTIES HEADER_FILE_ONLY TRUE)

  list(APPEND viewer_SOURCE_FILES ${viewer_INSTALLER_FILES})
endif (WINDOWS)

if (OPENAL)
  set(LLSTARTUP_COMPILE_FLAGS "${LLSTARTUP_COMPILE_FLAGS} -DLL_OPENAL")
endif (OPENAL)

if (FMODEX)
  set(LLSTARTUP_COMPILE_FLAGS "${LLSTARTUP_COMPILE_FLAGS} -DLL_FMODEX")
  set(FMODWRAPPER_LIBRARY ${FMODEX_LIBRARY})
endif (FMODEX)

set_source_files_properties(llstartup.cpp PROPERTIES COMPILE_FLAGS "${LLSTARTUP_COMPILE_FLAGS}")

list(APPEND viewer_SOURCE_FILES ${viewer_HEADER_FILES})

set_source_files_properties(${viewer_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

add_executable(${VIEWER_BINARY_NAME}
    WIN32
    MACOSX_BUNDLE
    ${viewer_SOURCE_FILES}
    )

if (SDL_FOUND)
  set_property(TARGET ${VIEWER_BINARY_NAME}
    PROPERTY COMPILE_DEFINITIONS LL_SDL=1
    )
endif (SDL_FOUND)

# add package files
file(GLOB EVENT_HOST_SCRIPT_GLOB_LIST
     ${CMAKE_CURRENT_SOURCE_DIR}/../viewer_components/*.py)
list(APPEND EVENT_HOST_SCRIPTS ${EVENT_HOST_SCRIPT_GLOB_LIST})

set(PACKAGE ON CACHE BOOL
    "Add a package target that builds an installer package.")

if (WINDOWS)
    set_target_properties(${VIEWER_BINARY_NAME}
        PROPERTIES
        # *TODO -reenable this once we get server usage sorted out
        LINK_FLAGS "/debug /NODEFAULTLIB:LIBCMT /SUBSYSTEM:WINDOWS ${TCMALLOC_LINK_FLAGS} /LARGEADDRESSAWARE"
        LINK_FLAGS_DEBUG "/NODEFAULTLIB:\"LIBCMT;LIBCMTD;MSVCRT\" /INCREMENTAL:NO /LARGEADDRESSAWARE"
        LINK_FLAGS_RELEASE "/FORCE:MULTIPLE /MAP\"secondlife-bin.MAP\" /OPT:REF /LARGEADDRESSAWARE"
        )
    if(USE_PRECOMPILED_HEADERS)
        set_target_properties(
            ${VIEWER_BINARY_NAME}
            PROPERTIES
            COMPILE_FLAGS "/Yullviewerprecompiledheaders.h"
            )
    endif(USE_PRECOMPILED_HEADERS)

    # If adding a file to viewer_manifest.py in the WindowsManifest.construct() method, be sure to add the dependency
    # here.
    # *NOTE:Mani - This is a crappy hack to have important dependencies for the viewer_manifest copy action
    # be met. I'm looking forward to a source-code split-up project next year that will address this kind of thing.
    # In the meantime, if you have any ideas on how to easily maintain one list, either here or in viewer_manifest.py
    # and have the build deps get tracked *please* tell me about it.

    if(USE_TCMALLOC)
      # Configure a var for tcmalloc location, if used.
      # Note the need to specify multiple names explicitly.
      set(GOOGLE_PERF_TOOLS_SOURCE
        ${SHARED_LIB_STAGING_DIR}/Release/libtcmalloc_minimal.dll
        ${SHARED_LIB_STAGING_DIR}/RelWithDebInfo/libtcmalloc_minimal.dll
        ${SHARED_LIB_STAGING_DIR}/Debug/libtcmalloc_minimal-debug.dll
        )
     endif(USE_TCMALLOC)


    set(COPY_INPUT_DEPENDENCIES
      # The following commented dependencies are determined at variably at build time. Can't do this here.
      #${SHARED_LIB_STAGING_DIR}/${CMAKE_CFG_INTDIR}/libtcmalloc_minimal.dll => None ... Skipping libtcmalloc_minimal.dll
      ${CMAKE_SOURCE_DIR}/../etc/message.xml
      ${CMAKE_SOURCE_DIR}/../scripts/messages/message_template.msg
      ${SHARED_LIB_STAGING_DIR}/${CMAKE_CFG_INTDIR}/llcommon.dll
      ${SHARED_LIB_STAGING_DIR}/${CMAKE_CFG_INTDIR}/libapr-1.dll
      ${SHARED_LIB_STAGING_DIR}/${CMAKE_CFG_INTDIR}/libaprutil-1.dll
      ${SHARED_LIB_STAGING_DIR}/${CMAKE_CFG_INTDIR}/libapriconv-1.dll
      ${SHARED_LIB_STAGING_DIR}/Release/glod.dll
      ${SHARED_LIB_STAGING_DIR}/RelWithDebInfo/glod.dll
      ${SHARED_LIB_STAGING_DIR}/Debug/glod.dll
      ${SHARED_LIB_STAGING_DIR}/Release/libcollada14dom22.dll
      ${SHARED_LIB_STAGING_DIR}/RelWithDebInfo/libcollada14dom22.dll
      ${SHARED_LIB_STAGING_DIR}/Debug/libcollada14dom22-d.dll
      ${SHARED_LIB_STAGING_DIR}/Release/openjpeg.dll
      ${SHARED_LIB_STAGING_DIR}/RelWithDebInfo/openjpeg.dll
      ${SHARED_LIB_STAGING_DIR}/Debug/openjpegd.dll
      ${SHARED_LIB_STAGING_DIR}/Release/msvcr100.dll
      ${SHARED_LIB_STAGING_DIR}/Release/msvcp100.dll
      ${SHARED_LIB_STAGING_DIR}/RelWithDebInfo/msvcr100.dll
      ${SHARED_LIB_STAGING_DIR}/RelWithDebInfo/msvcp100.dll
      ${SHARED_LIB_STAGING_DIR}/Debug/msvcr100d.dll
      ${SHARED_LIB_STAGING_DIR}/Debug/msvcp100d.dll
      ${SHARED_LIB_STAGING_DIR}/Release/libhunspell.dll
      ${SHARED_LIB_STAGING_DIR}/RelWithDebInfo/libhunspell.dll
      ${SHARED_LIB_STAGING_DIR}/Debug/libhunspell.dll
      ${SHARED_LIB_STAGING_DIR}/${CMAKE_CFG_INTDIR}/SLVoice.exe
      ${SHARED_LIB_STAGING_DIR}/${CMAKE_CFG_INTDIR}/vivoxsdk.dll
      ${SHARED_LIB_STAGING_DIR}/${CMAKE_CFG_INTDIR}/ortp.dll
      ${SHARED_LIB_STAGING_DIR}/${CMAKE_CFG_INTDIR}/libsndfile-1.dll
      ${SHARED_LIB_STAGING_DIR}/${CMAKE_CFG_INTDIR}/zlib1.dll
      ${SHARED_LIB_STAGING_DIR}/${CMAKE_CFG_INTDIR}/vivoxplatform.dll
      ${SHARED_LIB_STAGING_DIR}/${CMAKE_CFG_INTDIR}/vivoxoal.dll
      ${SHARED_LIB_STAGING_DIR}/${CMAKE_CFG_INTDIR}/ca-bundle.crt
      ${GOOGLE_PERF_TOOLS_SOURCE}
      ${CMAKE_CURRENT_SOURCE_DIR}/licenses-win32.txt
      ${CMAKE_CURRENT_SOURCE_DIR}/featuretable.txt
      ${CMAKE_CURRENT_SOURCE_DIR}/featuretable_xp.txt
      ${ARCH_PREBUILT_DIRS_RELEASE}/libeay32.dll
      ${ARCH_PREBUILT_DIRS_RELEASE}/qtcore4.dll
      ${ARCH_PREBUILT_DIRS_RELEASE}/qtgui4.dll
      ${ARCH_PREBUILT_DIRS_RELEASE}/qtnetwork4.dll
      ${ARCH_PREBUILT_DIRS_RELEASE}/qtopengl4.dll
      ${ARCH_PREBUILT_DIRS_RELEASE}/qtwebkit4.dll
      ${ARCH_PREBUILT_DIRS_RELEASE}/qtxmlpatterns4.dll
      ${ARCH_PREBUILT_DIRS_RELEASE}/ssleay32.dll
      ${ARCH_PREBUILT_DIRS_RELEASE}/imageformats/qgif4.dll
      ${ARCH_PREBUILT_DIRS_RELEASE}/imageformats/qico4.dll
      ${ARCH_PREBUILT_DIRS_RELEASE}/imageformats/qjpeg4.dll
      ${ARCH_PREBUILT_DIRS_RELEASE}/imageformats/qmng4.dll
      ${ARCH_PREBUILT_DIRS_RELEASE}/imageformats/qsvg4.dll
      ${ARCH_PREBUILT_DIRS_RELEASE}/imageformats/qtiff4.dll
      ${ARCH_PREBUILT_DIRS_RELEASE}/codecs/qcncodecs4.dll
      ${ARCH_PREBUILT_DIRS_RELEASE}/codecs/qjpcodecs4.dll
      ${ARCH_PREBUILT_DIRS_RELEASE}/codecs/qkrcodecs4.dll
      ${ARCH_PREBUILT_DIRS_RELEASE}/codecs/qtwcodecs4.dll
      ${ARCH_PREBUILT_DIRS_DEBUG}/libeay32.dll
      ${ARCH_PREBUILT_DIRS_DEBUG}/qtcored4.dll
      ${ARCH_PREBUILT_DIRS_DEBUG}/qtguid4.dll
      ${ARCH_PREBUILT_DIRS_DEBUG}/qtnetworkd4.dll
      ${ARCH_PREBUILT_DIRS_DEBUG}/qtopengld4.dll
      ${ARCH_PREBUILT_DIRS_DEBUG}/qtwebkitd4.dll
      ${ARCH_PREBUILT_DIRS_DEBUG}/qtxmlpatternsd4.dll
      ${ARCH_PREBUILT_DIRS_DEBUG}/ssleay32.dll
      ${ARCH_PREBUILT_DIRS_DEBUG}/imageformats/qgifd4.dll
      ${ARCH_PREBUILT_DIRS_DEBUG}/imageformats/qicod4.dll
      ${ARCH_PREBUILT_DIRS_DEBUG}/imageformats/qjpegd4.dll
      ${ARCH_PREBUILT_DIRS_DEBUG}/imageformats/qmngd4.dll
      ${ARCH_PREBUILT_DIRS_DEBUG}/imageformats/qsvgd4.dll
      ${ARCH_PREBUILT_DIRS_DEBUG}/imageformats/qtiffd4.dll
      ${ARCH_PREBUILT_DIRS_RELEASE}/codecs/qcncodecsd4.dll
      ${ARCH_PREBUILT_DIRS_RELEASE}/codecs/qjpcodecsd4.dll
      ${ARCH_PREBUILT_DIRS_RELEASE}/codecs/qkrcodecsd4.dll
      ${ARCH_PREBUILT_DIRS_RELEASE}/codecs/qtwcodecsd4.dll
      SLPlugin
      media_plugin_quicktime
      media_plugin_webkit
      winmm_shim
      windows-crash-logger
      windows-updater
      )

    if (FMODEX)
      list(APPEND COPY_INPUT_DEPENDENCIES
           ${SHARED_LIB_STAGING_DIR}/Release/fmodex.dll
           ${SHARED_LIB_STAGING_DIR}/RelWithDebInfo/fmodex.dll
           ${SHARED_LIB_STAGING_DIR}/Debug/fmodexL.dll
          )
    endif (FMODEX)
    
    add_custom_command(
      OUTPUT  ${CMAKE_CFG_INTDIR}/copy_touched.bat
      COMMAND ${PYTHON_EXECUTABLE}
      ARGS
        ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
        --actions=copy
        --arch=${ARCH}
        --artwork=${ARTWORK_DIR}
        --build=${CMAKE_CURRENT_BINARY_DIR}
        --buildtype=${CMAKE_BUILD_TYPE}
        --configuration=${CMAKE_CFG_INTDIR}
        --dest=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}
        --grid=${GRID}
        --channel=${VIEWER_CHANNEL}
        --versionfile=${CMAKE_CURRENT_BINARY_DIR}/viewer_version.txt
        --source=${CMAKE_CURRENT_SOURCE_DIR}
        --touch=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/copy_touched.bat
      DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
        generate_viewer_version
        stage_third_party_libs
        ${COPY_INPUT_DEPENDENCIES}
      COMMENT "Performing viewer_manifest copy"
      )

    add_custom_target(copy_w_viewer_manifest ALL DEPENDS ${CMAKE_CFG_INTDIR}/copy_touched.bat)

    add_dependencies(${VIEWER_BINARY_NAME} stage_third_party_libs llcommon copy_w_viewer_manifest)

    if (EXISTS ${CMAKE_SOURCE_DIR}/copy_win_scripts)
      add_dependencies(${VIEWER_BINARY_NAME} copy_win_scripts)
    endif (EXISTS ${CMAKE_SOURCE_DIR}/copy_win_scripts)

    add_dependencies(${VIEWER_BINARY_NAME}
      SLPlugin
      windows-updater
      windows-crash-logger
      )

    # sets the 'working directory' for debugging from visual studio.
    if (NOT UNATTENDED)
        add_custom_command(
            TARGET ${VIEWER_BINARY_NAME} POST_BUILD
            COMMAND ${CMAKE_SOURCE_DIR}/tools/vstool/vstool.exe
            ARGS
              --solution
              ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.sln
              --workingdir
              ${VIEWER_BINARY_NAME}
              "${CMAKE_CURRENT_SOURCE_DIR}"
            COMMENT "Setting the ${VIEWER_BINARY_NAME} working directory for debugging."
            )
    endif (NOT UNATTENDED)

    if (PACKAGE)
      add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/event_host.tar.bz2
        COMMAND ${PYTHON_EXECUTABLE}
        ARGS
          ${CMAKE_CURRENT_SOURCE_DIR}/event_host_manifest.py
          ${CMAKE_CURRENT_SOURCE_DIR}/..
          ${CMAKE_CURRENT_BINARY_DIR}
          ${CMAKE_CFG_INTDIR}
        DEPENDS
          lleventhost
          ${EVENT_HOST_SCRIPTS}
          ${CMAKE_CURRENT_SOURCE_DIR}/event_host_manifest.py
        )

      add_custom_command(
        OUTPUT ${CMAKE_CFG_INTDIR}/touched.bat
        COMMAND ${PYTHON_EXECUTABLE}
        ARGS
          ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
          --arch=${ARCH}
          --artwork=${ARTWORK_DIR}
          --build=${CMAKE_CURRENT_BINARY_DIR}
          --buildtype=${CMAKE_BUILD_TYPE}
          --channel=${VIEWER_CHANNEL}
          --versionfile=${CMAKE_CURRENT_BINARY_DIR}/viewer_version.txt
          --configuration=${CMAKE_CFG_INTDIR}
          --dest=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}
          --grid=${GRID}
          --source=${CMAKE_CURRENT_SOURCE_DIR}
          --touch=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/touched.bat
        DEPENDS
            ${VIEWER_BINARY_NAME}
            ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
            ${CMAKE_CURRENT_BINARY_DIR}/viewer_version.txt
            ${COPY_INPUT_DEPENDENCIES}
        )

      add_custom_target(package ALL DEPENDS
        ${CMAKE_CFG_INTDIR}/touched.bat
        windows-setup-build-all
        generate_viewer_version
        )
        # temporarily disable packaging of event_host until hg subrepos get
        # sorted out on the parabuild cluster...
        #${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/event_host.tar.bz2)

    endif (PACKAGE)
elseif (DARWIN)
    set_target_properties(${VIEWER_BINARY_NAME}
        PROPERTIES
        LINK_FLAGS_RELEASE "${LINK_FLAGS_RELEASE} -Xlinker -dead_strip -Xlinker -map -Xlinker ${CMAKE_CURRENT_BINARY_DIR}/${VIEWER_BINARY_NAME}.MAP"
        )
else (WINDOWS)
        # Linux
    set_target_properties(${VIEWER_BINARY_NAME}
        PROPERTIES
        LINK_FLAGS_RELEASE "${LINK_FLAGS_RELEASE} -Wl,--Map=${VIEWER_BINARY_NAME}.MAP"
        )
endif (WINDOWS)

# *NOTE - this list is very sensitive to ordering, test carefully on all
# platforms if you change the releative order of the entries here.
# In particular, cmake 2.6.4 (when buidling with linux/makefile generators)
# appears to sometimes de-duplicate redundantly listed dependencies improperly.
# To work around this, higher level modules should be listed before the modules
# that they depend upon. -brad
target_link_libraries(${VIEWER_BINARY_NAME}
    ${UPDATER_LIBRARIES}
    ${GOOGLE_PERFTOOLS_LIBRARIES}
    ${LLAUDIO_LIBRARIES}
    ${LLCHARACTER_LIBRARIES}
    ${LLIMAGE_LIBRARIES}
    ${LLINVENTORY_LIBRARIES}
    ${LLMESSAGE_LIBRARIES}
    ${LLPLUGIN_LIBRARIES}
    ${LLPRIMITIVE_LIBRARIES}
    ${LLRENDER_LIBRARIES}
    ${FREETYPE_LIBRARIES}
    ${LLUI_LIBRARIES}
    ${LLVFS_LIBRARIES}
    ${LLWINDOW_LIBRARIES}
    ${LLXML_LIBRARIES}
    ${LSCRIPT_LIBRARIES}
    ${LLMATH_LIBRARIES}
    ${LLCOREHTTP_LIBRARIES}
    ${LLCOMMON_LIBRARIES}
    ${NDOF_LIBRARY}
    ${NVAPI_LIBRARY}
    ${HUNSPELL_LIBRARY}
    ${viewer_LIBRARIES}
    ${BOOST_PROGRAM_OPTIONS_LIBRARY}
    ${BOOST_REGEX_LIBRARY}
    ${BOOST_CONTEXT_LIBRARY}
    ${DBUSGLIB_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${FMODWRAPPER_LIBRARY} # must come after LLAudio
    ${GLOD_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${JSONCPP_LIBRARIES}
    ${SDL_LIBRARY}
    ${SMARTHEAP_LIBRARY}
    ${UI_LIBRARIES}
    ${WINDOWS_LIBRARIES}
    ${EXPAT_LIBRARIES}
    ${XMLRPCEPI_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${CRYPTO_LIBRARIES}
    ${LLLOGIN_LIBRARIES}
    ${LLPHYSICS_LIBRARIES}
    ${LLPHYSICSEXTENSIONS_LIBRARIES}
    ${TCMALLOC_LIBRARIES}
    ${LLAPPEARANCE_LIBRARIES}
    )

set(ARTWORK_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH
    "Path to artwork files.")

if (LINUX)
  set(product SecondLife-${ARCH}-${VIEWER_SHORT_VERSION}.${VIEWER_VERSION_REVISION})

  # These are the generated targets that are copied to package/
  set(COPY_INPUT_DEPENDENCIES
    ${VIEWER_BINARY_NAME}
    linux-crash-logger
    SLPlugin
    media_plugin_webkit
    media_plugin_gstreamer010
    llcommon
    )

  add_custom_command(
      OUTPUT ${product}.tar.bz2
      COMMAND ${PYTHON_EXECUTABLE}
      ARGS
        ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
        --arch=${ARCH}
        --artwork=${ARTWORK_DIR}
        --build=${CMAKE_CURRENT_BINARY_DIR}
        --buildtype=${CMAKE_BUILD_TYPE}
        --channel=${VIEWER_CHANNEL}
        --versionfile=${CMAKE_CURRENT_BINARY_DIR}/viewer_version.txt
        --configuration=${CMAKE_CFG_INTDIR}
        --dest=${CMAKE_CURRENT_BINARY_DIR}/packaged
        --grid=${GRID}
        --source=${CMAKE_CURRENT_SOURCE_DIR}
        --touch=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/.${product}.touched
      DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
        generate_viewer_version
        ${COPY_INPUT_DEPENDENCIES}
      )

  if (PACKAGE)
  endif (PACKAGE)

  add_custom_command(
    OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/.${product}.copy_touched
    COMMAND ${PYTHON_EXECUTABLE}
    ARGS
      ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
      --arch=${ARCH}
      --actions=copy
      --artwork=${ARTWORK_DIR}
      --build=${CMAKE_CURRENT_BINARY_DIR}
      --buildtype=${CMAKE_BUILD_TYPE}
      --configuration=${CMAKE_CFG_INTDIR}
      --dest=${CMAKE_CURRENT_BINARY_DIR}/packaged
      --grid=${GRID}
      --channel=${VIEWER_CHANNEL}
      --versionfile=${CMAKE_CURRENT_BINARY_DIR}/viewer_version.txt
      --source=${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS
      ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
      generate_viewer_version
      ${COPY_INPUT_DEPENDENCIES}
    COMMENT "Performing viewer_manifest copy"
    )

  add_custom_target(copy_l_viewer_manifest ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/.${product}.copy_touched)

  if (PACKAGE)
    add_custom_target(package ALL DEPENDS ${product}.tar.bz2)
    # Make sure we don't run two instances of viewer_manifest.py at the same time.
    add_dependencies(package copy_l_viewer_manifest)
    check_message_template(package)
  endif (PACKAGE)
endif (LINUX)

if (DARWIN)
  # These all get set with PROPERTIES
  set(product "Second Life")
  set(MACOSX_BUNDLE_INFO_STRING "Second Life Viewer")
  set(MACOSX_BUNDLE_ICON_FILE "secondlife.icns")
  set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.secondlife.indra.viewer")
  set(MACOSX_BUNDLE_LONG_VERSION_STRING "${VIEWER_CHANNEL} ${VIEWER_SHORT_VERSION}.${VIEWER_VERSION_REVISION}")
  set(MACOSX_BUNDLE_BUNDLE_NAME "SecondLife")
  set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${VIEWER_SHORT_VERSION}")
  set(MACOSX_BUNDLE_BUNDLE_VERSION "${VIEWER_SHORT_VERSION}${VIEWER_MACOSX_PHASE}${VIEWER_REVISION}")
  set(MACOSX_BUNDLE_COPYRIGHT "Copyright © Linden Research, Inc. 2007")
  set(MACOSX_BUNDLE_NSMAIN_NIB_FILE "SecondLife.nib")
  set(MACOSX_BUNDLE_NSPRINCIPAL_CLASS "NSApplication")
  
  set_target_properties(
    ${VIEWER_BINARY_NAME}
    PROPERTIES
    OUTPUT_NAME "${product}"
    MACOSX_BUNDLE_INFO_PLIST
    "${CMAKE_CURRENT_SOURCE_DIR}/Info-SecondLife.plist"
    )

  configure_file(
     "${CMAKE_CURRENT_SOURCE_DIR}/Info-SecondLife.plist"
     "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${product}.app/Contents/Info.plist"
    )

  add_custom_command(
    TARGET ${VIEWER_BINARY_NAME} POST_BUILD
    COMMAND ${PYTHON_EXECUTABLE}
    ARGS
      ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
      --actions=copy
      --arch=${ARCH}
      --artwork=${ARTWORK_DIR}
      --build=${CMAKE_CURRENT_BINARY_DIR}
      --buildtype=${CMAKE_BUILD_TYPE}
      --configuration=${CMAKE_CFG_INTDIR}
      --dest=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${product}.app
      --grid=${GRID}
      --channel=${VIEWER_CHANNEL}
      --versionfile=${CMAKE_CURRENT_BINARY_DIR}/viewer_version.txt
      --source=${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS
      ${VIEWER_BINARY_NAME}
      ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
      generate_viewer_version
    )

  add_dependencies(${VIEWER_BINARY_NAME} SLPlugin media_plugin_quicktime media_plugin_webkit mac-crash-logger)

  if (ENABLE_SIGNING)
      set(SIGNING_SETTING "--signature=${SIGNING_IDENTITY}")
  else (ENABLE_SIGNING)
      set(SIGNING_SETTING "")
  endif (ENABLE_SIGNING)

  if (PACKAGE)
      add_custom_target(package ALL DEPENDS ${VIEWER_BINARY_NAME})
      add_dependencies(package generate_viewer_version)

      add_custom_command(
        TARGET package POST_BUILD
        COMMAND ${PYTHON_EXECUTABLE}
        ARGS
          ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
          --arch=${ARCH}
          --artwork=${ARTWORK_DIR}
          --build=${CMAKE_CURRENT_BINARY_DIR}
          --buildtype=${CMAKE_BUILD_TYPE}
          --channel=${VIEWER_CHANNEL}
          --configuration=${CMAKE_CFG_INTDIR}
          --dest=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${product}.app
          --grid=${GRID}
          --channel=${VIEWER_CHANNEL}
          --versionfile=${CMAKE_CURRENT_BINARY_DIR}/viewer_version.txt
          --source=${CMAKE_CURRENT_SOURCE_DIR}
          --touch=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/.${product}.touched
          ${SIGNING_SETTING}
        DEPENDS
          ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
          generate_viewer_version
      )
  endif (PACKAGE)
endif (DARWIN)

if (INSTALL)
  include(${CMAKE_CURRENT_SOURCE_DIR}/ViewerInstall.cmake)
endif (INSTALL)

if (PACKAGE)
  set(SYMBOL_SEARCH_DIRS "")
  if (WINDOWS)
    list(APPEND SYMBOL_SEARCH_DIRS "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}")
    set(VIEWER_SYMBOL_FILE "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/secondlife-symbols-windows.tar.bz2")
    # slplugin.exe failing symbols dump - need to debug, might have to do with updated version of google breakpad
    # set(VIEWER_EXE_GLOBS "${VIEWER_BINARY_NAME}${CMAKE_EXECUTABLE_SUFFIX} slplugin.exe")
    set(VIEWER_EXE_GLOBS "${VIEWER_BINARY_NAME}${CMAKE_EXECUTABLE_SUFFIX}")
    set(VIEWER_LIB_GLOB "*${CMAKE_SHARED_MODULE_SUFFIX}")
    set(VIEWER_COPY_MANIFEST copy_w_viewer_manifest)
  endif (WINDOWS)
  if (DARWIN)
    list(APPEND SYMBOL_SEARCH_DIRS "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}")
    # *TODO: Generate these search dirs in the cmake files related to each binary.
    list(APPEND SYMBOL_SEARCH_DIRS "${CMAKE_BINARY_DIR}/llplugin/slplugin/${CMAKE_CFG_INTDIR}")
    list(APPEND SYMBOL_SEARCH_DIRS "${CMAKE_BINARY_DIR}/mac_crash_logger/${CMAKE_CFG_INTDIR}")
    list(APPEND SYMBOL_SEARCH_DIRS "${CMAKE_BINARY_DIR}/media_plugins/gstreamer010/${CMAKE_CFG_INTDIR}")
    list(APPEND SYMBOL_SEARCH_DIRS "${CMAKE_BINARY_DIR}/media_plugins/quicktime/${CMAKE_CFG_INTDIR}")
    list(APPEND SYMBOL_SEARCH_DIRS "${CMAKE_BINARY_DIR}/media_plugins/webkit/${CMAKE_CFG_INTDIR}")
    set(VIEWER_SYMBOL_FILE "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/secondlife-symbols-darwin.tar.bz2")
    set(VIEWER_EXE_GLOBS "'Second Life' SLPlugin mac-crash-logger")
    set(VIEWER_LIB_GLOB "*.dylib")
  endif (DARWIN)
  if (LINUX)
    list(APPEND SYMBOL_SEARCH_DIRS "${CMAKE_CURRENT_BINARY_DIR}/packaged")
    set(VIEWER_SYMBOL_FILE "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/secondlife-symbols-linux.tar.bz2")
    set(VIEWER_EXE_GLOBS "do-not-directly-run-secondlife-bin SLPlugin")
    set(VIEWER_LIB_GLOB "*${CMAKE_SHARED_MODULE_SUFFIX}*")
    set(VIEWER_COPY_MANIFEST copy_l_viewer_manifest)
  endif (LINUX)

  if(RELEASE_CRASH_REPORTING OR NON_RELEASE_CRASH_REPORTING)
  if(CMAKE_CFG_INTDIR STREQUAL ".")
      set(LLBUILD_CONFIG ${CMAKE_BUILD_TYPE})
  else(CMAKE_CFG_INTDIR STREQUAL ".")
      # set LLBUILD_CONFIG to be a shell variable evaluated at build time
      # reflecting the configuration we are currently building.
      set(LLBUILD_CONFIG ${CMAKE_CFG_INTDIR})
  endif(CMAKE_CFG_INTDIR STREQUAL ".")
  add_custom_command(OUTPUT "${VIEWER_SYMBOL_FILE}"
    COMMAND "${PYTHON_EXECUTABLE}"
    ARGS
      "${CMAKE_CURRENT_SOURCE_DIR}/generate_breakpad_symbols.py"
      "${LLBUILD_CONFIG}"
      "${SYMBOL_SEARCH_DIRS}"
      "${VIEWER_EXE_GLOBS}"
      "${VIEWER_LIB_GLOB}"
      "${AUTOBUILD_INSTALL_DIR}/bin/dump_syms"
      "${VIEWER_SYMBOL_FILE}"
    DEPENDS generate_breakpad_symbols.py
        VERBATIM)

  add_custom_target(generate_breakpad_symbols DEPENDS "${VIEWER_SYMBOL_FILE}")
  add_dependencies(generate_breakpad_symbols "${VIEWER_BINARY_NAME}" "${VIEWER_COPY_MANIFEST}")
  add_dependencies(package generate_breakpad_symbols)
  endif(RELEASE_CRASH_REPORTING OR NON_RELEASE_CRASH_REPORTING)
endif (PACKAGE)

if (LL_TESTS)
  # To add a viewer unit test, just add the test .cpp file below
  # This creates a separate test project per file listed.
  include(LLAddBuildTest)
  SET(viewer_TEST_SOURCE_FILES
    llagentaccess.cpp
    lldateutil.cpp
    llmediadataclient.cpp
    lllogininstance.cpp
    llremoteparcelrequest.cpp
    lltranslate.cpp
    llviewerhelputil.cpp
    llversioninfo.cpp
    llworldmap.cpp
    llworldmipmap.cpp
  )

  set_source_files_properties(
    llworldmap.cpp
    llworldmipmap.cpp
    PROPERTIES
    LL_TEST_ADDITIONAL_SOURCE_FILES 
    tests/llviewertexture_stub.cpp
    #llviewertexturelist.cpp
  )

  set_source_files_properties(
    lltranslate.cpp
    PROPERTIES
    LL_TEST_ADDITIONAL_LIBRARIES "${JSONCPP_LIBRARIES}"
  )

  set_source_files_properties(
    llviewerhelputil.cpp
    PROPERTIES
    LL_TEST_ADDITIONAL_LIBRARIES "${BOOST_SYSTEM_LIBRARY}"
  )

  set_source_files_properties(
    llremoteparcelrequest.cpp
    PROPERTIES
    LL_TEST_ADDITIONAL_LIBRARIES "${BOOST_SYSTEM_LIBRARY}"
  )

  set_source_files_properties(
    llworldmap.cpp
    llworldmipmap.cpp
    PROPERTIES
    LL_TEST_ADDITIONAL_SOURCE_FILES 
    tests/llviewertexture_stub.cpp
    #llviewertexturelist.cpp
    LL_TEST_ADDITIONAL_LIBRARIES "${BOOST_SYSTEM_LIBRARY}"
  )

  set_source_files_properties(
    llmediadataclient.cpp
    PROPERTIES
    LL_TEST_ADDITIONAL_LIBRARIES "${LLPRIMITIVE_LIBRARIES}"
  )

  set_source_files_properties(
    llagentaccess.cpp
    PROPERTIES
    LL_TEST_ADDITIONAL_LIBRARIES "${BOOST_SYSTEM_LIBRARY}"
  )

  set_source_files_properties(
    lllogininstance.cpp
    PROPERTIES
    LL_TEST_ADDITIONAL_LIBRARIES "${BOOST_SYSTEM_LIBRARY}"
  )

  ##################################################
  # DISABLING PRECOMPILED HEADERS USAGE FOR TESTS
  ##################################################
  # if(USE_PRECOMPILED_HEADERS)
  #     set_source_files_properties(
  #       ${viewer_TEST_SOURCE_FILES}
  #       PROPERTIES
  #         LL_TEST_ADDITIONAL_SOURCE_FILES llviewerprecompiledheaders.cpp
  #       )
  # endif(USE_PRECOMPILED_HEADERS)
  LL_ADD_PROJECT_UNIT_TESTS(${VIEWER_BINARY_NAME} "${viewer_TEST_SOURCE_FILES}")

  #set(TEST_DEBUG on)
  set(test_sources llcapabilitylistener.cpp)
  ##################################################
  # DISABLING PRECOMPILED HEADERS USAGE FOR TESTS
  ##################################################
  # if(USE_PRECOMPILED_HEADERS)
  #     set(test_sources "${test_sources}" llviewerprecompiledheaders.cpp)
  # endif(USE_PRECOMPILED_HEADERS)
  set(test_libs
    ${LLMESSAGE_LIBRARIES}
    ${WINDOWS_LIBRARIES}
    ${LLVFS_LIBRARIES}
    ${LLMATH_LIBRARIES}
    ${LLCOMMON_LIBRARIES}
    ${GOOGLEMOCK_LIBRARIES}
    )

  LL_ADD_INTEGRATION_TEST(llcapabilitylistener
    "${test_sources}"
    "${test_libs}"
    ${PYTHON_EXECUTABLE}
    "${CMAKE_SOURCE_DIR}/llmessage/tests/test_llsdmessage_peer.py"
    )

  set(test_libs
    ${LLMESSAGE_LIBRARIES}
    ${WINDOWS_LIBRARIES}
    ${LLVFS_LIBRARIES}
    ${LLMATH_LIBRARIES}
    ${LLCOMMON_LIBRARIES}
    ${GOOGLEMOCK_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${CRYPTO_LIBRARIES}
  )

    LL_ADD_INTEGRATION_TEST(llsechandler_basic
    llsechandler_basic.cpp
    "${test_libs}"
    )

  LL_ADD_INTEGRATION_TEST(llsecapi
     llsecapi.cpp
    "${test_libs}"
    )

  set(llslurl_test_sources
      llslurl.cpp
      llviewernetwork.cpp
  )


  LL_ADD_INTEGRATION_TEST(llslurl
     "${llslurl_test_sources}"
    "${test_libs}"
    )

  LL_ADD_INTEGRATION_TEST(llviewernetwork
     llviewernetwork.cpp
    "${test_libs}"
    )

  LL_ADD_INTEGRATION_TEST(llviewerassetstats
    llviewerassetstats.cpp
    "${test_libs}"
    )

  #ADD_VIEWER_BUILD_TEST(llmemoryview viewer)
  #ADD_VIEWER_BUILD_TEST(llagentaccess viewer)
  #ADD_VIEWER_BUILD_TEST(lltextureinfo viewer)
  #ADD_VIEWER_BUILD_TEST(lltextureinfodetails viewer)
  #ADD_VIEWER_BUILD_TEST(lltexturestatsuploader viewer)

include(LLAddBuildTest)
SET(viewer_TEST_SOURCE_FILES
  llagentaccess.cpp
  llwlparammanager.cpp
  # Not *actually* a unit test, it's an integration test.
  # Because it won't work in the new unit test iface, i've commented out
  # and notified Nat. Delete this when it's replaced!
  # + poppy & brad 2009-06-05
  # llcapabilitylistener.cpp
  )
set_source_files_properties(
  ${viewer_TEST_SOURCE_FILES}
  PROPERTIES
    LL_TEST_ADDITIONAL_SOURCE_FILES llviewerprecompiledheaders.cpp
  )

endif (LL_TESTS)

check_message_template(${VIEWER_BINARY_NAME})

