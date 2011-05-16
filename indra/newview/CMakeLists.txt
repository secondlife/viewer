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
include(FMOD)
include(OPENAL)
include(FindOpenGL)
include(JsonCpp)
include(LLAudio)
include(LLCharacter)
include(LLCommon)
include(LLConvexDecomposition)
include(LLImage)
include(LLImageJ2COJ)
include(LLInventory)
include(LLMath)
include(LLMessage)
include(LLPlugin)
include(LLPrimitive)
include(LLRender)
include(LLUI)
include(LLVFS)
include(LLWindow)
include(LLXML)
include(LLXUIXML)
include(LScript)
include(Linking)
include(NDOF)
include(GooglePerfTools)
include(TemplateCheck)
include(UI)
include(UnixInstall)
include(LLKDU)
include(ViewerMiscLibs)
include(LLLogin)
include(GLOD)
include(CMakeCopyIfDifferent)

include_directories(
    ${DBUSGLIB_INCLUDE_DIRS}
    ${JSONCPP_INCLUDE_DIRS}
    ${GLOD_INCLUDE_DIR}
    ${LLAUDIO_INCLUDE_DIRS}
    ${LLCHARACTER_INCLUDE_DIRS}
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLCONVEXDECOMP_INCLUDE_DIRS}
    ${FMOD_INCLUDE_DIR}
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
    ${LLXUIXML_INCLUDE_DIRS}
    ${LSCRIPT_INCLUDE_DIRS}
    ${LSCRIPT_INCLUDE_DIRS}/lscript_compile
    ${LLLOGIN_INCLUDE_DIRS}
    ${UPDATER_INCLUDE_DIRS}
    ${LIBS_PREBUILT_DIR}/include/collada
    ${OPENAL_LIB_INCLUDE_DIRS}
    ${LIBS_PREBUILT_DIR}/include/collada/1.4
    )

set(viewer_SOURCE_FILES
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
    llappearancemgr.cpp
    llappviewer.cpp
    llappviewerlistener.cpp
    llassetuploadqueue.cpp
    llassetuploadresponders.cpp
    llattachmentsmgr.cpp
    llaudiosourcevo.cpp
    llavataractions.cpp
    llavatariconctrl.cpp
    llavatarlist.cpp
    llavatarlistitem.cpp
    llavatarpropertiesprocessor.cpp
    llbottomtray.cpp
    llbox.cpp
    llbreadcrumbview.cpp
    llbrowsernotification.cpp
    llbuycurrencyhtml.cpp
    llcallbacklist.cpp
    llcallfloater.cpp
    llcallingcard.cpp
    llcapabilitylistener.cpp
    llcaphttpsender.cpp
    llchannelmanager.cpp
    llchatbar.cpp
    llchathistory.cpp
    llchatitemscontainerctrl.cpp
    llchatmsgbox.cpp
    llchiclet.cpp
    llclassifiedinfo.cpp
    llclassifiedstatsresponder.cpp
    llcloud.cpp
    llcofwearables.cpp
    llcolorswatch.cpp
    llcommanddispatcherlistener.cpp
    llcommandhandler.cpp
    llcommandlineparser.cpp
    llcompilequeue.cpp
    llconfirmationmanager.cpp
    llcurrencyuimanager.cpp
    llcylinder.cpp
    lldateutil.cpp
    lldebugmessagebox.cpp
    lldebugview.cpp
    lldelayedgestureerror.cpp
    lldirpicker.cpp
    lldndbutton.cpp
    lldrawable.cpp
    lldrawpool.cpp
    lldrawpoolalpha.cpp
    lldrawpoolavatar.cpp
    lldrawpoolbump.cpp
    lldrawpoolground.cpp
    lldrawpoolsimple.cpp
    lldrawpoolsky.cpp
    lldrawpoolterrain.cpp
    lldrawpooltree.cpp
    lldrawpoolwater.cpp
    lldrawpoolwlsky.cpp
    lldriverparam.cpp
    lldynamictexture.cpp
    llemote.cpp
    lleventnotifier.cpp
    lleventpoll.cpp
    llexpandabletextbox.cpp
    llexternaleditor.cpp
    llface.cpp
    llfasttimerview.cpp
    llfavoritesbar.cpp
    llfeaturemanager.cpp
    llfilepicker.cpp
    llfilteredwearablelist.cpp
    llfirstuse.cpp
    llflexibleobject.cpp
    llfloaterabout.cpp
    llfloateranimpreview.cpp
    llfloaterauction.cpp
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
    llfloatercolorpicker.cpp
    llfloaterdaycycle.cpp
    llfloaterdisplayname.cpp
    llfloaterenvsettings.cpp
    llfloaterevent.cpp
    llfloaterfonttest.cpp
    llfloatergesture.cpp
    llfloatergodtools.cpp
    llfloatergroupinvite.cpp
    llfloatergroups.cpp
    llfloaterhandler.cpp
    llfloaterhardwaresettings.cpp
    llfloaterhelpbrowser.cpp
    llfloaterhud.cpp
    llfloaterimagepreview.cpp
    llfloaterinspect.cpp
    llfloaterinventory.cpp
    llfloaterjoystick.cpp
    llfloaterlagmeter.cpp
    llfloaterland.cpp
    llfloaterlandholdings.cpp
    llfloatermap.cpp
    llfloatermediabrowser.cpp
    llfloatermediasettings.cpp
    llfloatermemleak.cpp
    llfloatermodelpreview.cpp
    llfloatermodelwizard.cpp
    llfloaternamedesc.cpp
    llfloaternotificationsconsole.cpp
    llfloateropenobject.cpp
    llfloaterpay.cpp
    llfloaterperms.cpp
    llfloaterpostcard.cpp
    llfloaterpostprocess.cpp
    llfloaterpreference.cpp
    llfloaterproperties.cpp
    llfloaterregiondebugconsole.cpp
    llfloaterregioninfo.cpp
    llfloaterreporter.cpp
    llfloaterscriptdebug.cpp
    llfloaterscriptlimits.cpp
    llfloatersearch.cpp
    llfloatersellland.cpp
    llfloatersettingsdebug.cpp
    llfloatersidetraytab.cpp
    llfloatersnapshot.cpp
    llfloatersounddevices.cpp
    llfloatertelehub.cpp
    llfloatertestinspectors.cpp
    llfloatertestlistview.cpp
    llfloatertools.cpp
    llfloatertopobjects.cpp
    llfloatertos.cpp
    llfloateruipreview.cpp
    llfloaterurlentry.cpp
    llfloatervoiceeffect.cpp
    llfloaterwater.cpp
    llfloaterwebcontent.cpp
    llfloaterwhitelistentry.cpp
    llfloaterwindlight.cpp
    llfloaterwindowsize.cpp
    llfloaterworldmap.cpp
    llfolderview.cpp
    llfolderviewitem.cpp
    llfollowcam.cpp
    llfriendcard.cpp
    llgesturemgr.cpp
    llgiveinventory.cpp
    llglsandbox.cpp
    llgroupactions.cpp
    llgroupiconctrl.cpp
    llgrouplist.cpp
    llgroupmgr.cpp
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
    llimfloater.cpp
    llimfloatercontainer.cpp
    llimhandler.cpp
    llimview.cpp
    llinspect.cpp
    llinspectavatar.cpp
    llinspectgroup.cpp
    llinspectobject.cpp
    llinspectremoteobject.cpp
    llinspecttoast.cpp
    llinventorybridge.cpp
    llinventoryclipboard.cpp
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
    lllocaltextureobject.cpp
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
    llmediactrl.cpp
    llmediadataclient.cpp
    llmemoryview.cpp
    llmenucommands.cpp
    llmeshrepository.cpp
    llmimetypes.cpp
    llmorphview.cpp
    llmoveview.cpp
    llmutelist.cpp
    llnamebox.cpp
    llnameeditor.cpp
    llnamelistctrl.cpp
    llnavigationbar.cpp
    llnearbychat.cpp
    llnearbychatbar.cpp
    llnearbychathandler.cpp
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
    llpanelimcontrolpanel.cpp
    llpanelland.cpp
    llpanellandaudio.cpp
    llpanellandmarkinfo.cpp
    llpanellandmarks.cpp
    llpanellandmedia.cpp
    llpanellogin.cpp
    llpanelloginlistener.cpp
    llpanelmaininventory.cpp
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
    llpanelprofileview.cpp
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
    llphysicsmotion.cpp
    llphysicsshapebuilderutil.cpp
    llplacesinventorybridge.cpp
    llplacesinventorypanel.cpp
    llpopupview.cpp
    llpolymesh.cpp
    llpolymorph.cpp
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
    llregionposition.cpp
    llremoteparcelrequest.cpp
    llsavedsettingsglue.cpp
    llsaveoutfitcombobtn.cpp
    llsceneview.cpp
    llscreenchannel.cpp
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
    llsidetray.cpp
    llsidetraylistener.cpp
    llsidetraypanelcontainer.cpp
    llsky.cpp
    llslurl.cpp
    llspatialpartition.cpp
    llspeakbutton.cpp
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
    llsyswellitem.cpp
    llsyswellwindow.cpp
    llteleporthistory.cpp
    llteleporthistorystorage.cpp
    lltexglobalcolor.cpp
    lltexlayer.cpp
    lltexlayerparams.cpp
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
    lltool.cpp
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
    llviewerjointmesh_sse.cpp
    llviewerjointmesh_sse2.cpp
    llviewerjointmesh_vec.cpp
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
    llviewertexteditor.cpp
    llviewertexture.cpp
    llviewertextureanim.cpp
    llviewertexturelist.cpp
    llviewerthrottle.cpp
    llviewervisualparam.cpp
    llviewerwindow.cpp
    llviewerwindowlistener.cpp
    llvlcomposition.cpp
    llvlmanager.cpp
    llvoavatar.cpp
    llvoavatardefines.cpp
    llvoavatarself.cpp
    llvocache.cpp
    llvoclouds.cpp
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
    llvotextbubble.cpp
    llvotree.cpp
    llvovolume.cpp
    llvowater.cpp
    llvowlsky.cpp
    llwatchdog.cpp
    llwaterparammanager.cpp
    llwaterparamset.cpp
    llwearable.cpp
    llwearableitemslist.cpp
    llwearablelist.cpp
    llwearabletype.cpp
    llweb.cpp
    llwebsharing.cpp
    llwind.cpp
    llwlanimator.cpp
    llwldaycycle.cpp
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

if (LINUX)
  # We can't set these flags for Darwin, because they get passed to
  # the PPC compiler.  Ugh.

  set_source_files_properties(
      llviewerjointmesh_sse.cpp
      PROPERTIES COMPILE_FLAGS "-msse -mfpmath=sse"
      )
  set_source_files_properties(
      llviewerjointmesh_sse2.cpp
      PROPERTIES COMPILE_FLAGS "-msse2 -mfpmath=sse"
      )
endif (LINUX)

set(viewer_HEADER_FILES
    CMakeLists.txt
    ViewerInstall.cmake
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
    llappearance.h
    llappearancemgr.h
    llappviewer.h
    llappviewerlistener.h
    llassetuploadqueue.h
    llassetuploadresponders.h
    llattachmentsmgr.h
    llaudiosourcevo.h
    llavataractions.h
    llavatariconctrl.h
    llavatarlist.h
    llavatarlistitem.h
    llavatarpropertiesprocessor.h
    llbottomtray.h
    llbox.h
    llbreadcrumbview.h
    llbuycurrencyhtml.h
    llcallbacklist.h
    llcallfloater.h
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
    llclassifiedinfo.h
    llclassifiedstatsresponder.h
    llcloud.h
    llcofwearables.h
    llcolorswatch.h
    llcommanddispatcherlistener.h
    llcommandhandler.h
    llcommandlineparser.h
    llcompilequeue.h
    llconfirmationmanager.h
    llcurrencyuimanager.h
    llcylinder.h
    lldateutil.h
    lldebugmessagebox.h
    lldebugview.h
    lldelayedgestureerror.h
    lldirpicker.h
    lldndbutton.h
    lldrawable.h
    lldrawpool.h
    lldrawpoolalpha.h
    lldrawpoolavatar.h
    lldrawpoolbump.h
    lldrawpoolclouds.h
    lldrawpoolground.h
    lldrawpoolsimple.h
    lldrawpoolsky.h
    lldrawpoolterrain.h
    lldrawpooltree.h
    lldrawpoolwater.h
    lldrawpoolwlsky.h
    lldriverparam.h
    lldynamictexture.h
    llemote.h
    lleventnotifier.h
    lleventpoll.h
    llexpandabletextbox.h
    llexternaleditor.h
    llface.h
    llfasttimerview.h
    llfavoritesbar.h
    llfeaturemanager.h
    llfilepicker.h
    llfilteredwearablelist.h
    llfirstuse.h
    llflexibleobject.h
    llfloaterabout.h
    llfloateranimpreview.h
    llfloaterauction.h
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
    llfloatercolorpicker.h
    llfloaterdaycycle.h
    llfloaterdisplayname.h
    llfloaterenvsettings.h
    llfloaterevent.h
    llfloaterfonttest.h
    llfloatergesture.h
    llfloatergodtools.h
    llfloatergroupinvite.h
    llfloatergroups.h
    llfloaterhandler.h
    llfloaterhardwaresettings.h
    llfloaterhelpbrowser.h
    llfloaterhud.h
    llfloaterimagepreview.h
    llfloaterinspect.h
    llfloaterinventory.h
    llfloaterjoystick.h
    llfloaterlagmeter.h
    llfloaterland.h
    llfloaterlandholdings.h
    llfloatermap.h
    llfloatermediabrowser.h
    llfloatermediasettings.h
    llfloatermemleak.h
    llfloatermodelpreview.h
    llfloatermodelwizard.h
    llfloaternamedesc.h
    llfloaternotificationsconsole.h
    llfloateropenobject.h
    llfloaterpay.h
    llfloaterperms.h
    llfloaterpostcard.h
    llfloaterpostprocess.h
    llfloaterpreference.h
    llfloaterproperties.h
    llfloaterregiondebugconsole.h
    llfloaterregioninfo.h
    llfloaterreporter.h
    llfloaterscriptdebug.h
    llfloaterscriptlimits.h
    llfloatersearch.h
    llfloatersellland.h
    llfloatersettingsdebug.h
    llfloatersidetraytab.h
    llfloatersnapshot.h
    llfloatersounddevices.h
    llfloatertelehub.h
    llfloatertestinspectors.h
    llfloatertestlistview.h
    llfloatertools.h
    llfloatertopobjects.h
    llfloatertos.h
    llfloateruipreview.h
    llfloaterurlentry.h
    llfloatervoiceeffect.h
    llfloaterwater.h
    llfloaterwebcontent.h
    llfloaterwhitelistentry.h
    llfloaterwindlight.h
    llfloaterwindowsize.h
    llfloaterworldmap.h
    llfolderview.h
    llfoldervieweventlistener.h
    llfolderviewitem.h
    llfollowcam.h
    llfriendcard.h
    llgesturemgr.h
    llgiveinventory.h
    llgroupactions.h
    llgroupiconctrl.h
    llgrouplist.h
    llgroupmgr.h
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
    llimfloater.h
    llimfloatercontainer.h
    llimview.h
    llinspect.h
    llinspectavatar.h
    llinspectgroup.h
    llinspectobject.h
    llinspectremoteobject.h
    llinspecttoast.h
    llinventorybridge.h
    llinventoryclipboard.h
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
    lllocaltextureobject.h
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
    llmediactrl.h
    llmediadataclient.h
    llmemoryview.h
    llmenucommands.h
    llmeshrepository.h
    llmimetypes.h
    llmorphview.h
    llmoveview.h
    llmutelist.h
    llnamebox.h
    llnameeditor.h
    llnamelistctrl.h
    llnavigationbar.h
    llnearbychat.h
    llnearbychatbar.h
    llnearbychathandler.h
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
    llpanelimcontrolpanel.h
    llpanelland.h
    llpanellandaudio.h
    llpanellandmarkinfo.h
    llpanellandmarks.h
    llpanellandmedia.h
    llpanellogin.h
    llpanelloginlistener.h
    llpanelmaininventory.h
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
    llpanelprofileview.h
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
    llphysicsmotion.h
    llphysicsshapebuilderutil.h
    llplacesinventorybridge.h
    llplacesinventorypanel.h
    llpolymesh.h
    llpolymorph.h
    llpopupview.h
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
    llregionposition.h
    llremoteparcelrequest.h
    llresourcedata.h
    llrootview.h
    llsavedsettingsglue.h
    llsaveoutfitcombobtn.h
    llsceneview.h
    llscreenchannel.h
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
    llsidetray.h
    llsidetraylistener.h
    llsidetraypanelcontainer.h
    llsky.h
    llslurl.h
    llspatialpartition.h
    llspeakbutton.h
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
    llsyswellitem.h
    llsyswellwindow.h    
    lltable.h
    llteleporthistory.h
    llteleporthistorystorage.h
    lltexglobalcolor.h
    lltexlayer.h
    lltexlayerparams.h
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
    lltool.h
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
    llviewertexteditor.h
    llviewertexture.h
    llviewertextureanim.h
    llviewertexturelist.h
    llviewerthrottle.h
    llviewervisualparam.h
    llviewerwindow.h
    llviewerwindowlistener.h
    llvlcomposition.h
    llvlmanager.h
    llvoavatar.h
    llvoavatardefines.h
    llvoavatarself.h
    llvocache.h
    llvoclouds.h
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
    llvotextbubble.h
    llvotree.h
    llvotreenew.h
    llvovolume.h
    llvowater.h
    llvowlsky.h
    llwatchdog.h
    llwaterparammanager.h
    llwaterparamset.h
    llwearable.h
    llwearableitemslist.h
    llwearablelist.h
    llwearabletype.h
    llweb.h
    llwebsharing.h
    llwind.h
    llwlanimator.h
    llwldaycycle.h
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
    VertexCache.h
    VorbisFramework.h
    )

source_group("CMake Rules" FILES ViewerInstall.cmake)

if (DARWIN)
  LIST(APPEND viewer_SOURCE_FILES llappviewermacosx.cpp)

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
    SecondLife.nib/
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
        res/lltoolpipette.cur
        res/lltoolrotate.cur
        res/lltoolscale.cur
        res/lltooltranslate.cur
        res/lltoolzoomin.cur
        res/lltoolzoomout.cur
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

    set(viewer_RESOURCE_FILES 
        res/viewerRes.rc 
        ${viewer_RESOURCE_FILES}
        )

    SOURCE_GROUP("Resource Files" FILES ${viewer_RESOURCE_FILES})

    if (NOT STANDALONE)
        list(APPEND viewer_SOURCE_FILES ${viewer_RESOURCE_FILES})
    endif (NOT STANDALONE)      

    find_library(DINPUT_LIBRARY dinput8 ${DIRECTX_LIBRARY_DIR})
    find_library(DXGUID_LIBRARY dxguid ${DIRECTX_LIBRARY_DIR})
    mark_as_advanced(
        DINPUT_LIBRARY
        DXGUID_LIBRARY
        )

    set(viewer_LIBRARIES
        ${WINDOWS_LIBRARIES}
        comdlg32
        ${DINPUT_LIBRARY}
        ${DXGUID_LIBRARY}
        kernel32
        odbc32
        odbccp32
        ole32
        oleaut32
        opengl32
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
    skins/minimal/colors.xml
    skins/minimal/textures/textures.xml



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
    app_settings/grass.xml
    app_settings/high_graphics.xml
    app_settings/ignorable_dialogs.xml
    app_settings/keys.xml
    app_settings/keywords.ini
    app_settings/logcontrol.xml
    app_settings/low_graphics.xml
    app_settings/mid_graphics.xml
    app_settings/settings.xml
    app_settings/settings_crash_behavior.xml
    app_settings/settings_files.xml
    app_settings/settings_per_account.xml
    app_settings/settings_minimal.xml
    app_settings/std_bump.ini
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
if (NOT STANDALONE)
    list(APPEND viewer_SOURCE_FILES ${viewer_CHARACTER_FILES})
endif (NOT STANDALONE)

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

if (FMOD)
  set(LLSTARTUP_COMPILE_FLAGS "${LLSTARTUP_COMPILE_FLAGS} -DLL_FMOD")

  if (DARWIN)
    set(fmodwrapper_SOURCE_FILES fmodwrapper.cpp)
    add_library(fmodwrapper SHARED ${fmodwrapper_SOURCE_FILES})
    set(fmodwrapper_needed_LIBRARIES ${FMOD_LIBRARY} ${CARBON_LIBRARY})
    set_target_properties(
      fmodwrapper
      PROPERTIES
      BUILD_WITH_INSTALL_RPATH 1
      INSTALL_NAME_DIR "@executable_path/../Resources"
      LINK_FLAGS "-unexported_symbols_list ${CMAKE_CURRENT_SOURCE_DIR}/fmod_hidden_symbols.exp"
      )
    set(FMODWRAPPER_LIBRARY fmodwrapper)
    target_link_libraries(fmodwrapper ${fmodwrapper_needed_LIBRARIES})
  else (DARWIN)
    # fmodwrapper unnecessary on linux or windows
    set(FMODWRAPPER_LIBRARY ${FMOD_LIBRARY})
  endif (DARWIN)
endif (FMOD)

set_source_files_properties(llstartup.cpp PROPERTIES COMPILE_FLAGS "${LLSTARTUP_COMPILE_FLAGS}")

list(APPEND viewer_SOURCE_FILES ${viewer_HEADER_FILES})

set_source_files_properties(${viewer_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

add_executable(${VIEWER_BINARY_NAME}
    WIN32
    MACOSX_BUNDLE
    ${viewer_SOURCE_FILES}
    )

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
        #LINK_FLAGS "/debug /NODEFAULTLIB:LIBCMT /SUBSYSTEM:WINDOWS /INCLUDE:\"__tcmalloc\""
        LINK_FLAGS "/debug /NODEFAULTLIB:LIBCMT /SUBSYSTEM:WINDOWS /INCLUDE:__tcmalloc"
        LINK_FLAGS_DEBUG "/NODEFAULTLIB:\"LIBCMT;LIBCMTD;MSVCRT\" /INCREMENTAL:NO"
        LINK_FLAGS_RELEASE ""
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

    if(USE_GOOGLE_PERFTOOLS)
      # Configure a var for tcmalloc location, if used.
      # Note the need to specify multiple names explicitly.
      set(GOOGLE_PERF_TOOLS_SOURCE
        ${SHARED_LIB_STAGING_DIR}/Release/libtcmalloc_minimal.dll
        ${SHARED_LIB_STAGING_DIR}/RelWithDebInfo/libtcmalloc_minimal.dll
        ${SHARED_LIB_STAGING_DIR}/Debug/libtcmalloc_minimal-debug.dll
        )
     endif(USE_GOOGLE_PERFTOOLS)
 

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
      ${SHARED_LIB_STAGING_DIR}/Release/fmod.dll
      ${SHARED_LIB_STAGING_DIR}/RelWithDebInfo/fmod.dll
      ${SHARED_LIB_STAGING_DIR}/Debug/fmod.dll
      ${SHARED_LIB_STAGING_DIR}/Release/msvcr100.dll
      ${SHARED_LIB_STAGING_DIR}/Release/msvcp100.dll
      ${SHARED_LIB_STAGING_DIR}/RelWithDebInfo/msvcr100.dll
      ${SHARED_LIB_STAGING_DIR}/RelWithDebInfo/msvcp100.dll
      ${SHARED_LIB_STAGING_DIR}/Debug/msvcr100d.dll
      ${SHARED_LIB_STAGING_DIR}/Debug/msvcp100d.dll
      ${SHARED_LIB_STAGING_DIR}/${CMAKE_CFG_INTDIR}/SLVoice.exe
      ${SHARED_LIB_STAGING_DIR}/${CMAKE_CFG_INTDIR}/vivoxsdk.dll
      ${SHARED_LIB_STAGING_DIR}/${CMAKE_CFG_INTDIR}/ortp.dll
      ${SHARED_LIB_STAGING_DIR}/${CMAKE_CFG_INTDIR}/libsndfile-1.dll
      ${SHARED_LIB_STAGING_DIR}/${CMAKE_CFG_INTDIR}/zlib1.dll
      ${SHARED_LIB_STAGING_DIR}/${CMAKE_CFG_INTDIR}/vivoxplatform.dll
      ${SHARED_LIB_STAGING_DIR}/${CMAKE_CFG_INTDIR}/vivoxoal.dll
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

    add_custom_command(
      OUTPUT  ${CMAKE_CFG_INTDIR}/copy_touched.bat
      COMMAND ${PYTHON_EXECUTABLE}
      ARGS
        ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
        --actions=copy
        --artwork=${ARTWORK_DIR}
        --build=${CMAKE_CURRENT_BINARY_DIR}
        --buildtype=${CMAKE_BUILD_TYPE}
        --configuration=${CMAKE_CFG_INTDIR}
        --dest=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}
        --grid=${GRID}
        --source=${CMAKE_CURRENT_SOURCE_DIR}
        --touch=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/copy_touched.bat
      DEPENDS 
        ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
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
          --artwork=${ARTWORK_DIR}
          --build=${CMAKE_CURRENT_BINARY_DIR}
          --buildtype=${CMAKE_BUILD_TYPE}
          --channel=${VIEWER_CHANNEL}
          --configuration=${CMAKE_CFG_INTDIR}
          --dest=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}
          --grid=${GRID}
          --login_channel=${VIEWER_LOGIN_CHANNEL}
          --source=${CMAKE_CURRENT_SOURCE_DIR}
          --touch=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/touched.bat
        DEPENDS 
            ${VIEWER_BINARY_NAME} 
            ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
            ${COPY_INPUT_DEPENDENCIES}
        )

      add_custom_target(package ALL DEPENDS 
        ${CMAKE_CFG_INTDIR}/touched.bat
        windows-setup-build-all 
        )
        # temporarily disable packaging of event_host until hg subrepos get
        # sorted out on the parabuild cluster...
        #${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/event_host.tar.bz2)

    endif (PACKAGE)
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
    ${LLXUIXML_LIBRARIES}
    ${LSCRIPT_LIBRARIES}
    ${LLMATH_LIBRARIES}
    ${LLCOMMON_LIBRARIES}
    ${NDOF_LIBRARY}
    ${viewer_LIBRARIES}
    ${BOOST_PROGRAM_OPTIONS_LIBRARY}
    ${BOOST_REGEX_LIBRARY}
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
    ${LLCONVEXDECOMP_LIBRARY}
    ${TCMALLOC_LIBRARIES}
    )

if (USE_KDU)
    target_link_libraries(${VIEWER_BINARY_NAME}
        ${LLKDU_LIBRARIES}
        ${KDU_LIBRARY}
        )
else (USE_KDU)
    target_link_libraries(${VIEWER_BINARY_NAME}
        ${LLIMAGEJ2COJ_LIBRARIES}
        )
endif (USE_KDU)
    
build_version(viewer)

set(ARTWORK_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH
    "Path to artwork files.")


if (LINUX)
  set(product SecondLife-${ARCH}-${viewer_VERSION})

  # These are the generated targets that are copied to package/
  set(COPY_INPUT_DEPENDENCIES
	${VIEWER_BINARY_NAME}
	linux-crash-logger
	linux-updater
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
        --configuration=${CMAKE_CFG_INTDIR}
        --dest=${CMAKE_CURRENT_BINARY_DIR}/packaged
        --grid=${GRID}
        --installer_name=${product}
        --login_channel=${VIEWER_LOGIN_CHANNEL}
        --source=${CMAKE_CURRENT_SOURCE_DIR}
        --touch=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/.${product}.touched
      DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
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
      --source=${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS 
      ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
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
  set(product "Second Life")
  set_target_properties(
    ${VIEWER_BINARY_NAME}
    PROPERTIES
    OUTPUT_NAME "${product}"
    MACOSX_BUNDLE_INFO_STRING "info string - localize me"
    MACOSX_BUNDLE_ICON_FILE "secondlife.icns"
    MACOSX_BUNDLE_GUI_IDENTIFIER "Second Life"
    MACOSX_BUNDLE_LONG_VERSION_STRING "ververver"
    MACOSX_BUNDLE_BUNDLE_NAME "Second Life"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "asdf"
    MACOSX_BUNDLE_BUNDLE_VERSION "asdf"
    MACOSX_BUNDLE_COPYRIGHT "copyright linden lab 2007 - localize me and run me through a legal wringer"
    )

  add_custom_command(
    TARGET ${VIEWER_BINARY_NAME} POST_BUILD
    COMMAND ${PYTHON_EXECUTABLE}
    ARGS
      ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
      --actions=copy
      --artwork=${ARTWORK_DIR}
      --build=${CMAKE_CURRENT_BINARY_DIR}
      --buildtype=${CMAKE_BUILD_TYPE}
      --configuration=${CMAKE_CFG_INTDIR}
      --dest=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${product}.app
      --grid=${GRID}
      --source=${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${VIEWER_BINARY_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
    )

  add_dependencies(${VIEWER_BINARY_NAME} SLPlugin media_plugin_quicktime media_plugin_webkit mac-updater mac-crash-logger)

  if (PACKAGE)
      add_custom_target(package ALL DEPENDS ${VIEWER_BINARY_NAME}) 

      add_custom_command(
        TARGET package POST_BUILD
        COMMAND ${PYTHON_EXECUTABLE}
        ARGS
          ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
          --artwork=${ARTWORK_DIR}
          --build=${CMAKE_CURRENT_BINARY_DIR}
          --buildtype=${CMAKE_BUILD_TYPE}
          --channel=${VIEWER_CHANNEL}
          --configuration=${CMAKE_CFG_INTDIR}
          --dest=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${product}.app
          --grid=${GRID}
          --login_channel=${VIEWER_LOGIN_CHANNEL}
          --source=${CMAKE_CURRENT_SOURCE_DIR}
          --touch=${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/.${product}.touched
        DEPENDS
          ${CMAKE_CURRENT_SOURCE_DIR}/viewer_manifest.py
      )
  endif (PACKAGE)
endif (DARWIN)

if (INSTALL)
  include(${CMAKE_CURRENT_SOURCE_DIR}/ViewerInstall.cmake)
endif (INSTALL)

if (PACKAGE)
  if (WINDOWS)
    set(VIEWER_DIST_DIR "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}")
    set(VIEWER_SYMBOL_FILE "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/secondlife-symbols-windows.tar.bz2")
    # slplugin.exe failing symbols dump - need to debug, might have to do with updated version of google breakpad
    # set(VIEWER_EXE_GLOBS "${VIEWER_BINARY_NAME}${CMAKE_EXECUTABLE_SUFFIX} slplugin.exe")
    set(VIEWER_EXE_GLOBS "${VIEWER_BINARY_NAME}${CMAKE_EXECUTABLE_SUFFIX}")
    set(VIEWER_LIB_GLOB "*${CMAKE_SHARED_MODULE_SUFFIX}")
    set(VIEWER_COPY_MANIFEST copy_w_viewer_manifest)
  endif (WINDOWS)
  if (DARWIN)
    set(VIEWER_DIST_DIR "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${product}.app")
    set(VIEWER_SYMBOL_FILE "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/secondlife-symbols-darwin.tar.bz2")
    set(VIEWER_EXE_GLOBS "'Second Life' SLPlugin")
    set(VIEWER_LIB_GLOB "*.dylib")
  endif (DARWIN)
  if (LINUX)
    set(VIEWER_DIST_DIR "${CMAKE_CURRENT_BINARY_DIR}/packaged")
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
      "${VIEWER_DIST_DIR}"
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
    llviewerhelputil.cpp
    llversioninfo.cpp
    llworldmap.cpp
    llworldmipmap.cpp
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

  LL_ADD_INTEGRATION_TEST(llsimplestat
	""
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

endif (LL_TESTS)

check_message_template(${VIEWER_BINARY_NAME})

